"""Related artists + popularity. Deezer (no key) and Last.fm (optional free key)."""
from __future__ import annotations

import math

from .... import cache
from ....config import settings
from ....net import client

TTL = 30 * 24 * 3600


async def _cached_json(source: str, url: str, params: dict, ttl: int = TTL):
    key = cache.make_key(source, url, params)
    hit = cache.get(key)
    if hit is not None:
        return hit
    try:
        r = await client().get(url, params=params)
        if r.status_code != 200:
            return None
        data = r.json()
    except Exception:
        return None
    if isinstance(data, dict) and data.get("error"):
        return None
    cache.put(key, source, data, ttl)
    return data


async def deezer_artist(name: str) -> dict | None:
    data = await _cached_json("deezer", "https://api.deezer.com/search/artist", {"q": name, "limit": 8})
    if not data or not data.get("data"):
        return None
    norm = lambda s: "".join(ch for ch in s.lower() if ch.isalnum())
    exact = [a for a in data["data"] if norm(a["name"]) == norm(name)]
    # several artists can share a name: take the biggest exact match, else the biggest hit
    a = max(exact or data["data"][:3], key=lambda a: a.get("nb_fan", 0))
    return {"id": a["id"], "name": a["name"], "fans": a.get("nb_fan", 0), "picture": a.get("picture_medium")}


async def deezer_related(name: str, limit: int = 12) -> list[dict]:
    a = await deezer_artist(name)
    if not a:
        return []
    data = await _cached_json("deezer", f"https://api.deezer.com/artist/{a['id']}/related", {"limit": limit})
    if not data:
        return []
    return [{"name": x["name"], "fans": x.get("nb_fan", 0), "source": "deezer"} for x in data.get("data", [])[:limit]]


async def lastfm_similar(name: str, limit: int = 15) -> list[dict]:
    if not settings.has("LASTFM_API_KEY"):
        return []
    data = await _cached_json("lastfm", "https://ws.audioscrobbler.com/2.0/", {
        "method": "artist.getSimilar", "artist": name, "limit": limit, "autocorrect": 1,
        "api_key": settings.get("LASTFM_API_KEY"), "format": "json"})
    try:
        return [{"name": x["name"], "match": float(x.get("match", 0)), "source": "lastfm"}
                for x in data["similarartists"]["artist"]]
    except (TypeError, KeyError):
        return []


async def lastfm_listeners(name: str) -> int | None:
    if not settings.has("LASTFM_API_KEY"):
        return None
    data = await _cached_json("lastfm", "https://ws.audioscrobbler.com/2.0/", {
        "method": "artist.getInfo", "artist": name, "autocorrect": 1,
        "api_key": settings.get("LASTFM_API_KEY"), "format": "json"})
    try:
        return int(data["artist"]["stats"]["listeners"])
    except (TypeError, KeyError, ValueError):
        return None


async def popularity(name: str) -> dict:
    """0..1 popularity from Last.fm listeners or Deezer fans (log-scaled)."""
    listeners = await lastfm_listeners(name)
    if listeners:
        return {"value": listeners, "unit": "Last.fm-Hörer", "score": min(1.0, math.log10(listeners + 1) / 7)}
    a = await deezer_artist(name)
    if a and a["fans"]:
        return {"value": a["fans"], "unit": "Deezer-Fans", "score": min(1.0, math.log10(a["fans"] + 1) / 7.3)}
    return {"value": None, "unit": None, "score": 0.0}


async def related_artists(names: list[str], limit: int = 12) -> list[dict]:
    """Merge Last.fm + Deezer related artists for all seed artists."""
    seeds = {n.lower() for n in names}
    merged: dict[str, dict] = {}
    for n in names:
        for src in (await lastfm_similar(n), await deezer_related(n)):
            for i, x in enumerate(src):
                k = x["name"].lower()
                if k in seeds:
                    continue
                e = merged.setdefault(k, {"name": x["name"], "score": 0.0, "sources": set(), "via": set()})
                e["score"] += 1.0 / (1 + i * 0.3)
                e["sources"].add(x["source"])
                e["via"].add(n)
    out = sorted(merged.values(), key=lambda e: -e["score"])[:limit]
    return [{**e, "sources": sorted(e["sources"]), "via": sorted(e["via"])} for e in out]
