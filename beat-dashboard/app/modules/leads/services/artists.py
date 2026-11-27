"""Find rap artists from Vienna / Mödling with Instagram handles.

Sources: MusicBrainz (free, 1 req/s), Wikidata (free), YouTube channel descriptions (needs the YouTube key
from the SEO module). Instagram itself has no open search API - everything here is public metadata.
"""
from __future__ import annotations

import asyncio
import re
import time

from .... import cache
from ....config import settings
from ....net import client
from .pipeline import normalize_instagram

UA = {"User-Agent": "BeatDashboard/1.0 (local lead tool; +https://musicbrainz.org/doc/MusicBrainz_API)",
      "Accept": "application/json"}
MB = "https://musicbrainz.org/ws/2"
TAGS = '(tag:"hip hop" OR tag:hip-hop OR tag:rap OR tag:trap OR tag:"german hip hop" OR tag:"cloud rap" OR tag:drill OR tag:"austrian hip hop")'
MB_QUERIES = {
    "wien": [f"area:Vienna AND {TAGS}", f"beginarea:Vienna AND {TAGS}", f"area:Wien AND {TAGS}"],
    "moedling": [f'(area:Mödling OR beginarea:Mödling OR area:Perchtoldsdorf OR beginarea:Baden) AND {TAGS}'],
    "oesterreich": [f"area:Austria AND {TAGS}"],
}
RAP_TAGS = {"hip hop", "hip-hop", "rap", "trap", "german hip hop", "cloud rap", "drill", "austrian hip hop", "deutschrap",
            "gangsta rap", "conscious hip hop", "boom bap", "hiphop", "rnb", "r&b", "afrobeats", "pop rap", "emo rap"}
CORE_RAP = {"rap", "trap", "drill", "deutschrap", "german hip hop", "austrian hip hop", "gangsta rap", "cloud rap",
            "pop rap", "emo rap", "conscious hip hop"}
NOT_CLIENT_TAGS = {"lo-fi", "lofi", "lo-fi hip hop", "lofi hip hop", "chillhop", "jazz hop", "jazzhop", "instrumental hip hop",
                   "trip hop", "electronic", "techno", "house", "ambient", "downtempo", "beats"}
_mb_last = 0.0
_mb_lock = asyncio.Lock()


async def _mb(path: str, params: dict) -> dict | None:
    key = cache.make_key("musicbrainz", path, params)
    hit = cache.get(key)
    if hit is not None:
        return hit
    global _mb_last
    for attempt in range(3):
        async with _mb_lock:
            wait = 1.15 - (time.time() - _mb_last)
            if wait > 0:
                await asyncio.sleep(wait)
            _mb_last = time.time()
            try:
                r = await client().get(f"{MB}/{path}", params={**params, "fmt": "json"}, headers=UA, timeout=20)
            except Exception:
                r = None
        if r is not None and r.status_code == 200:
            data = r.json()
            cache.put(key, "musicbrainz", data, 14 * 24 * 3600)
            return data
        await asyncio.sleep(2 + attempt * 2)  # 503 "busy" -> back off
    return None


def _links(relations: list[dict]) -> dict:
    out = {}
    for rel in relations or []:
        url = (rel.get("url") or {}).get("resource", "")
        if "instagram.com" in url and "instagram" not in out:
            ig = normalize_instagram(url)
            if ig:
                out["instagram"] = ig
        elif "youtube.com" in url:
            out.setdefault("youtube", url)
        elif "spotify.com" in url:
            out.setdefault("spotify", url)
        elif "soundcloud.com" in url:
            out.setdefault("soundcloud", url)
        elif "tiktok.com" in url:
            out.setdefault("tiktok", url)
        elif rel.get("type") == "official homepage":
            out.setdefault("website", url)
    return out


async def musicbrainz(job, regions: list[str], limit_lookups: int = 120) -> list[dict]:
    found: dict[str, dict] = {}
    queries = [q for r in regions for q in MB_QUERIES.get(r, [])]
    for i, q in enumerate(queries):
        job.update(message=f"MusicBrainz-Suche {i + 1}/{len(queries)}")
        for offset in (0, 100):
            data = await _mb("artist", {"query": q, "limit": 100, "offset": offset})
            if not data:
                break
            for a in data.get("artists", []):
                if a.get("score", 0) < 60 or (a.get("life-span") or {}).get("ended"):
                    continue  # weak match, deceased or disbanded
                tags = {t["name"].lower() for t in a.get("tags", [])}
                if tags and not tags & RAP_TAGS:
                    continue
                if tags & NOT_CLIENT_TAGS and not tags & CORE_RAP:
                    continue  # lofi/instrumental producers don't need music videos
                found.setdefault(a["id"], {"name": a["name"], "area": (a.get("area") or {}).get("name"),
                                           "begin": (a.get("begin-area") or {}).get("name"),
                                           "tags": [t["name"] for t in a.get("tags", [])][:5], "type": a.get("type")})
            if len(data.get("artists", [])) < 100:
                break
    ids = list(found)[:limit_lookups]
    out = []
    for n, mbid in enumerate(ids):
        if job.cancelled:
            break
        job.update(done=n, total=len(ids), message=f"Links holen: {found[mbid]['name']}")
        d = await _mb(f"artist/{mbid}", {"inc": "url-rels"})
        links = _links((d or {}).get("relations", []))
        a = found[mbid]
        out.append({
            "kind": "artist", "name": a["name"], "source": "musicbrainz", "source_id": mbid,
            "instagram": links.get("instagram"), "website": links.get("website"),
            "district": a["area"] or a["begin"], "category": ", ".join(a["tags"]),
            "extra": {**{k: v for k, v in links.items() if k not in ("instagram", "website")},
                      "musicbrainz": f"https://musicbrainz.org/artist/{mbid}", "type": a["type"]},
        })
    job.update(done=len(ids), total=len(ids))
    return out


WIKIDATA_QUERY = """
SELECT DISTINCT ?p ?pLabel ?ig ?placeLabel ?yt ?sp WHERE {
  VALUES ?region { wd:Q1741 wd:Q277965 wd:Q676405 }   # Wien, Stadt Mödling, Bezirk Mödling
  { ?p wdt:P106 wd:Q2252262 } UNION { ?p wdt:P106 wd:Q130857 } UNION { ?p wdt:P136 wd:Q11401 } UNION { ?p wdt:P136 wd:Q438503 }
  { ?p wdt:P19 ?place } UNION { ?p wdt:P551 ?place } UNION { ?p wdt:P740 ?place }
  ?place wdt:P131* ?region .
  FILTER NOT EXISTS { ?p wdt:P570 ?died }
  FILTER NOT EXISTS { ?p wdt:P576 ?dissolved }
  OPTIONAL { ?p wdt:P2003 ?ig }
  OPTIONAL { ?p wdt:P2397 ?yt }
  OPTIONAL { ?p wdt:P1902 ?sp }
  SERVICE wikibase:label { bd:serviceParam wikibase:language "de,en". }
} LIMIT 500
"""


async def wikidata(job) -> list[dict]:
    job.update(message="Wikidata abfragen …")
    key = cache.make_key("wikidata", "sparql", {"q": WIKIDATA_QUERY})
    data = cache.get(key)
    if data is None:
        r = await client().post("https://query.wikidata.org/sparql", data={"query": WIKIDATA_QUERY},
                                headers={**UA, "Accept": "application/sparql-results+json"}, timeout=90)
        if r.status_code != 200:
            raise RuntimeError(f"Wikidata antwortet nicht (HTTP {r.status_code}) - in einer Minute nochmal probieren")
        data = r.json()
        cache.put(key, "wikidata", data, 7 * 24 * 3600)
    out: dict[str, dict] = {}
    for b in data["results"]["bindings"]:
        qid = b["p"]["value"].rsplit("/", 1)[-1]
        e = out.setdefault(qid, {
            "kind": "artist", "name": b["pLabel"]["value"], "source": "wikidata", "source_id": qid,
            "instagram": None, "district": b.get("placeLabel", {}).get("value"), "category": "Rap / Hip-Hop",
            "extra": {"wikidata": f"https://www.wikidata.org/wiki/{qid}"}})
        if b.get("ig") and not e["instagram"]:
            e["instagram"] = normalize_instagram(b["ig"]["value"])
        if b.get("yt"):
            e["extra"]["youtube"] = f"https://www.youtube.com/channel/{b['yt']['value']}"
        if b.get("sp"):
            e["extra"]["spotify"] = f"https://open.spotify.com/artist/{b['sp']['value']}"
    return list(out.values())


YT_QUERIES = ["wien rap", "rap aus wien", "wiener rap musikvideo", "austrorap", "deutschrap wien", "mödling rap",
              "wien drill", "wien trap musikvideo", "favoriten rap", "simmering rap", "ottakring rap", "österreich rap neu"]
IG_IN_TEXT = re.compile(r"(?:instagram\.com/|(?:ig|insta|instagram)\s*[:\-–]?\s*@?)([A-Za-z0-9_.]{2,30})", re.I)
EMAIL = re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}")


async def youtube_channels(job, queries: list[str] | None = None, max_searches: int = 8) -> list[dict]:
    """Up-and-coming acts: recent music videos -> their channels -> IG handle + booking mail in the description."""
    from ...seo.services import youtube as yt
    if not yt.available():
        raise RuntimeError("Dafür brauchst du den YouTube-API-Key (Einstellungen)")
    queries = (queries or YT_QUERIES)[:max_searches]
    channel_ids: dict[str, str] = {}
    from datetime import datetime, timedelta, timezone
    after = (datetime.now(timezone.utc) - timedelta(days=365)).strftime("%Y-%m-%dT00:00:00Z")
    for i, q in enumerate(queries):
        job.update(done=i, total=len(queries) + 1, message=f"YouTube-Suche: {q}")
        params = {"part": "snippet", "q": q, "type": "video", "videoCategoryId": "10", "maxResults": 50,
                  "regionCode": "AT", "relevanceLanguage": "de", "publishedAfter": after, "order": "relevance"}
        key = cache.make_key("yt_search", "leads", params)
        data = cache.get(key)
        if data is None:
            if yt.searches_left() <= 0:
                job.update(message="YouTube-Suchbudget für heute aufgebraucht")
                break
            data = await yt._get("search", params)
            cache.log_units("youtube_search", 1, "search.list(leads)")
            cache.put(key, "yt_search", data, 3 * 24 * 3600)
        for it in data.get("items", []):
            sn = it.get("snippet", {})
            if sn.get("channelId"):
                channel_ids.setdefault(sn["channelId"], sn.get("channelTitle", ""))
    job.update(message=f"{len(channel_ids)} Kanäle prüfen …")
    out = []
    ids = list(channel_ids)
    for start in range(0, len(ids), 50):
        chunk = ids[start:start + 50]
        data = await yt._get("channels", {"part": "snippet,statistics,brandingSettings", "id": ",".join(chunk), "maxResults": 50})
        cache.log_units("youtube", 1, "channels.list(leads)")
        for c in data.get("items", []):
            sn, st = c.get("snippet", {}), c.get("statistics", {})
            desc = (sn.get("description") or "") + "\n" + ((c.get("brandingSettings") or {}).get("channel", {}).get("description") or "")
            subs = int(st.get("subscriberCount", 0) or 0)
            country = sn.get("country")
            if country and country not in ("AT", "DE"):
                continue
            if subs > 300_000:  # labels / big acts - not realistic clients
                continue
            igs = [normalize_instagram(m) for m in IG_IN_TEXT.findall(desc)]
            igs = [x for x in igs if x]
            mails = [m.lower() for m in EMAIL.findall(desc)]
            out.append({
                "kind": "artist", "name": sn.get("title") or channel_ids.get(c["id"], ""), "source": "youtube",
                "source_id": c["id"], "instagram": igs[0] if igs else None, "email": mails[0] if mails else None,
                "district": "AT" if country == "AT" else (country or None), "category": "YouTube-Artist",
                "extra": {"youtube": f"https://www.youtube.com/channel/{c['id']}", "yt_subs": subs,
                          "yt_videos": int(st.get("videoCount", 0) or 0), "bio": desc.strip()[:400]},
            })
    job.update(done=len(queries) + 1, total=len(queries) + 1)
    return out
