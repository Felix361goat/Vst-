"""/api/seo/* - research, generation, history, upload tracking."""
from __future__ import annotations

import asyncio
import json
import re
import uuid
from pathlib import Path

from fastapi import APIRouter, File, HTTPException, UploadFile
from pydantic import BaseModel, Field

from ... import db
from ...config import UPLOAD_DIR, settings
from .services import audio, checks, generator, knowledge, llm, related, scoring, suggest, trending, youtube

router = APIRouter()
AUDIO_EXT = {".wav", ".mp3", ".flac", ".ogg", ".aif", ".aiff"}
MAX_AUDIO = 80 * 1024 * 1024


def my_subs() -> int:
    try:
        return int(settings.get("MY_SUBSCRIBERS", "0") or 0)
    except ValueError:
        return 0


# ---------------------------------------------------------------- knowledge
@router.get("/knowledge")
def get_knowledge():
    return {"genres": {k: {**v, "bpm": list(v["bpm"]), "sweet": list(v["sweet"])} for k, v in knowledge.GENRES.items()},
            "moods": knowledge.MOODS, "modifiers": knowledge.FORMAT_MODIFIERS, "year": knowledge.title_year(),
            "checklist": checks.UPLOAD_CHECKLIST, "stage": scoring.stage(my_subs()), "subs": my_subs()}


# ---------------------------------------------------------------- audio
@router.post("/audio/analyze")
async def analyze_audio(file: UploadFile = File(...)):
    ext = Path(file.filename or "").suffix.lower()
    if ext not in AUDIO_EXT:
        raise HTTPException(400, f"Dateityp {ext or '?'} nicht unterstützt. Erlaubt: {', '.join(sorted(AUDIO_EXT))}")
    data = await file.read()
    if len(data) > MAX_AUDIO:
        raise HTTPException(400, "Datei zu groß (max. 80 MB)")
    safe = re.sub(r"[^A-Za-z0-9._-]", "_", Path(file.filename).name)[-80:]
    path = UPLOAD_DIR / f"{uuid.uuid4().hex[:8]}_{safe}"
    path.write_bytes(data)
    try:
        result = await asyncio.to_thread(audio.analyze, path)
    except Exception as e:
        raise HTTPException(400, f"Audio konnte nicht gelesen werden: {e}")
    result["file"] = path.name
    result["genre_matches"] = knowledge.genre_for_bpm(result["bpm"])
    result["name_suggestion"] = Path(file.filename).stem[:40]
    return result


# ---------------------------------------------------------------- research building blocks
class Seed(BaseModel):
    q: str


@router.post("/suggest")
async def api_suggest(body: Seed):
    return {"suggestions": await suggest.suggest(body.q), "probe": await suggest.demand_probe(body.q)}


@router.post("/longtail")
async def api_longtail(body: Seed):
    return {"keywords": await suggest.long_tail(body.q.strip().lower())}


class Artists(BaseModel):
    artists: list[str]


@router.post("/related")
async def api_related(body: Artists):
    return {"related": await related.related_artists(body.artists)}


@router.get("/rising")
async def api_rising():
    return {"rising": await trending.rising_type_beat_artists()}


@router.post("/niche")
async def api_niche(body: Seed):
    try:
        rep = await youtube.niche_report(body.q.strip().lower())
    except youtube.QuotaExceeded as e:
        raise HTTPException(429, str(e))
    except youtube.YouTubeError as e:
        raise HTTPException(502, f"YouTube: {e}")
    if rep is None:
        raise HTTPException(400, "Kein YouTube-API-Key eingetragen (Einstellungen)")
    return rep


# ---------------------------------------------------------------- full analysis
class AnalyzeIn(BaseModel):
    artists: list[str] = Field(default_factory=list)
    genre: str = "trap"
    mood: str = "Dark"
    beat_name: str = ""
    bpm: float | None = None
    key: str | None = None
    free: str = "free"                 # free | free_for_profit | none
    sample_source: str = "original"    # original | royalty_free | cleared | uncleared
    modifier: str = ""
    use_youtube: bool = True
    youtube_depth: int = 4             # how many keywords get a (paid) YouTube niche check
    use_llm: bool = False
    find_related: bool = True
    audio_file: str | None = None


def _clean_artists(xs: list[str]) -> list[str]:
    out = []
    for x in xs:
        x = re.sub(r"\s+", " ", x).strip()
        if x and x.lower() not in [o.lower() for o in out]:
            out.append(x)
    return out[:4]


async def _research(inp: AnalyzeIn) -> dict:
    artists = _clean_artists(inp.artists)
    g = knowledge.GENRES.get(inp.genre)
    notes: list[str] = []

    rising_list = await trending.rising(limit=200, min_listeners=500_000)
    rising_names = {r["artist"].lower(): r for r in rising_list}

    rel = await related.related_artists(artists) if (artists and inp.find_related) else []
    pool: list[tuple[str, str]] = [(a, "deine Wahl") for a in artists]
    pool += [(r["name"], "ähnlich wie " + ", ".join(r["via"])) for r in rel[:8]]
    if g:
        pool += [(a, f"typisch für {g['label']}") for a in g["artists"] if a.lower() not in {p[0].lower() for p in pool}][: max(3, 8 - len(artists))]

    candidates: list[dict] = []
    seen = set()
    for name, why in pool:
        phrase = f"{name.lower()} type beat"
        if phrase not in seen:
            seen.add(phrase)
            candidates.append({"phrase": phrase, "artist": name, "why": why})
    if len(artists) >= 2:
        candidates.insert(0, {"phrase": f"{artists[0].lower()} x {artists[1].lower()} type beat", "artist": artists[0], "why": "deine Kombo"})
    candidates = candidates[:14]
    gw = (g["label"].split("/")[0].strip().lower() if g else inp.genre)
    for phrase, why in ((f"{inp.mood.lower()} {gw} type beat", "Subgenre/Mood"), (f"{gw} type beat", "Genre")):
        if phrase not in seen:
            seen.add(phrase)
            candidates.append({"phrase": phrase, "artist": None, "why": why})
    candidates = candidates[:16]

    # demand (free) - in parallel, the suggest client rate-limits itself
    probes = await asyncio.gather(*(suggest.demand_probe(c["phrase"]) for c in candidates))
    pops = await asyncio.gather(*(related.popularity(c["artist"]) if c["artist"] else asyncio.sleep(0, {"score": 0.4, "value": None, "unit": None}) for c in candidates))
    for c, p, pop in zip(candidates, probes, pops):
        c["suggest"] = p
        c["popularity"] = pop
        c["rising"] = rising_names.get((c["artist"] or "").lower())
        c["prelim"] = scoring.score(suggest_score=p["score"], popularity=pop["score"], niche=None, artist=c["artist"],
                                    rising=bool(c["rising"]))

    # competition (YouTube) for the most promising + the user's own picks
    yt_used = False
    if inp.use_youtube and youtube.available():
        depth = max(0, min(inp.youtube_depth, youtube.searches_left() // 2, 8))
        if depth < inp.youtube_depth:
            notes.append(f"YouTube-Suchbudget knapp: nur {depth} Keywords live geprüft.")
        own = [c for c in candidates if c["why"] in ("deine Wahl", "deine Kombo")]
        rest = sorted([c for c in candidates if c not in own], key=lambda c: -c["prelim"]["opportunity"])
        for c in (own + rest)[:depth]:
            try:
                c["niche"] = await youtube.niche_report(c["phrase"])
                yt_used = True
            except youtube.QuotaExceeded as e:
                notes.append(f"YouTube-Kontingent erschöpft: {e}")
                break
            except youtube.YouTubeError as e:
                notes.append(f"YouTube-Fehler: {e}")
                break
    elif inp.use_youtube:
        notes.append("Ohne YouTube-API-Key ist die Konkurrenz nur geschätzt. Key unter Einstellungen eintragen (kostenlos).")

    for c in candidates:
        c["score"] = scoring.score(suggest_score=c["suggest"]["score"], popularity=c["popularity"]["score"],
                                   niche=c.get("niche"), artist=c["artist"], rising=bool(c["rising"]))
        c.pop("prelim", None)
        if c["artist"] and c["artist"].lower() in knowledge.HEAD_TERMS and my_subs() < 100_000:
            c["score"]["opportunity"] = round(c["score"]["opportunity"] * 0.6)
            c["score"]["label"] = "Zu groß"
            c["why"] += " · für Kanäle <100k kaum rankbar"
    candidates.sort(key=lambda c: -c["score"]["opportunity"])

    # keyword pool for tags: autocomplete expansions of the best phrases
    kw = []
    for c in candidates[:6]:
        kw += [c["phrase"]] + c["suggest"]["expansions"][:4]
    kw = list(dict.fromkeys(kw))

    # competitor tags + upload timing from YouTube data
    comp_tags: dict[str, int] = {}
    hours = [0] * 24
    days = [0] * 7
    for c in candidates:
        n = c.get("niche")
        if n:
            for t, cnt in n["top_tags"]:
                comp_tags[t] = comp_tags.get(t, 0) + cnt
            hours = [a + b for a, b in zip(hours, n["publish_hours_utc"])]
            days = [a + b for a, b in zip(days, n["publish_weekdays"])]

    return {"candidates": candidates, "related": rel, "keywords": kw,
            "competitor_tags": sorted(comp_tags.items(), key=lambda kv: -kv[1])[:25],
            "publish_hours_utc": hours, "publish_weekdays": days, "youtube_used": yt_used, "notes": notes}


def _generate(inp: AnalyzeIn, artists: list[str], keywords: list[str]) -> dict:
    gen = generator.build(
        artists=artists, genre=inp.genre, mood=inp.mood, beat_name=inp.beat_name,
        bpm=round(inp.bpm) if inp.bpm else None, key=inp.key, free=inp.free,
        producer=settings.get("PRODUCER_NAME"), store_link=settings.get("STORE_LINK"),
        contact=settings.get("INSTAGRAM"), keywords=keywords, modifier=inp.modifier, sample_source=inp.sample_source)
    first = gen["titles"][0]["text"] if gen["titles"] else ""
    gen["checks"] = checks.run(title=first, description=gen["description"], tags=gen["tags"], artists=artists,
                               genre=inp.genre, bpm=inp.bpm, free=inp.free, sample_source=inp.sample_source,
                               subs=my_subs(), store_link=settings.get("STORE_LINK"))
    return gen


@router.post("/analyze")
async def analyze(inp: AnalyzeIn):
    research = await _research(inp)
    artists = _clean_artists(inp.artists)
    recommendation = None
    best_artist = next((c for c in research["candidates"] if c["artist"]), None)
    if not artists and best_artist:
        artists = [best_artist["artist"]]
        recommendation = f"Kein Künstler angegeben → bestes Keyword: '{best_artist['phrase']}'"
    elif artists and best_artist and best_artist["artist"].lower() != artists[0].lower():
        own = next((c for c in research["candidates"] if c["artist"] and c["artist"].lower() == artists[0].lower()), None)
        if own and best_artist["score"]["opportunity"] - own["score"]["opportunity"] >= 15:
            recommendation = (f"'{best_artist['phrase']}' hat mehr Potenzial ({best_artist['score']['opportunity']}) als "
                              f"'{own['phrase']}' ({own['score']['opportunity']}). Klick in der Tabelle auf 'Verwenden', um umzuschalten.")
    gen = _generate(inp, artists, research["keywords"])

    ai = None
    if inp.use_llm and llm.available():
        ai = await llm.polish({
            "beat_name": inp.beat_name, "genre": inp.genre, "mood": inp.mood, "bpm": inp.bpm, "key": inp.key,
            "artists": artists, "license": inp.free, "producer": settings.get("PRODUCER_NAME"),
            "store_link": settings.get("STORE_LINK"), "year": knowledge.title_year(),
            "best_keywords": [{"phrase": c["phrase"], "opportunity": c["score"]["opportunity"]} for c in research["candidates"][:6]],
            "autocomplete": research["keywords"][:20],
            "competitor_titles": [v["title"] for c in research["candidates"] if c.get("niche") for v in c["niche"]["top_videos"][:4]][:12],
        })

    result = {**research, "generated": gen, "ai": ai, "artists_used": artists, "recommendation": recommendation,
              "stage": scoring.stage(my_subs()), "bpm_fit": knowledge.bpm_fit(inp.genre, inp.bpm)}
    with db.tx() as c:
        cur = c.execute(
            "INSERT INTO analyses(beat_name, genre, mood, bpm, musical_key, artists, audio_file, input, result) VALUES(?,?,?,?,?,?,?,?,?)",
            (inp.beat_name, inp.genre, inp.mood, inp.bpm, inp.key, json.dumps(artists), inp.audio_file,
             inp.model_dump_json(), json.dumps(result, ensure_ascii=False, default=str)))
        result["id"] = cur.lastrowid
    return result


class GenerateIn(AnalyzeIn):
    keywords: list[str] = Field(default_factory=list)


@router.post("/generate")
def generate(inp: GenerateIn):
    """Cheap re-generation (no API calls) - e.g. after switching the artist."""
    return _generate(inp, _clean_artists(inp.artists), inp.keywords)


class CheckIn(BaseModel):
    title: str
    description: str = ""
    tags: list[str] = Field(default_factory=list)
    artists: list[str] = Field(default_factory=list)
    genre: str = ""
    bpm: float | None = None
    free: str = "free"
    sample_source: str = "original"


@router.post("/check")
def check(body: CheckIn):
    return {"checks": checks.run(title=body.title, description=body.description, tags=body.tags, artists=body.artists,
                                 genre=body.genre, bpm=body.bpm, free=body.free, sample_source=body.sample_source,
                                 subs=my_subs(), store_link=settings.get("STORE_LINK"))}


# ---------------------------------------------------------------- history
@router.get("/analyses")
def list_analyses(limit: int = 50):
    with db.tx() as c:
        rows = c.execute("SELECT id, created_at, beat_name, genre, mood, bpm, musical_key, artists FROM analyses "
                         "ORDER BY id DESC LIMIT ?", (limit,)).fetchall()
    return [db.row_to_dict(r, ("artists",)) for r in rows]


@router.get("/analyses/{aid}")
def get_analysis(aid: int):
    with db.tx() as c:
        row = c.execute("SELECT * FROM analyses WHERE id=?", (aid,)).fetchone()
    if not row:
        raise HTTPException(404)
    return db.row_to_dict(row, ("artists", "input", "result"))


@router.delete("/analyses/{aid}")
def delete_analysis(aid: int):
    with db.tx() as c:
        c.execute("DELETE FROM analyses WHERE id=?", (aid,))
    return {"ok": True}


# ---------------------------------------------------------------- upload tracking
class UploadIn(BaseModel):
    analysis_id: int | None = None
    youtube_url: str = ""
    title: str = ""
    keyword: str = ""
    tags: str = ""
    published_at: str = ""
    notes: str = ""


def _video_id(s: str) -> str:
    m = re.search(r"(?:v=|youtu\.be/|shorts/|embed/)([A-Za-z0-9_-]{11})", s)
    if m:
        return m.group(1)
    return s.strip() if re.fullmatch(r"[A-Za-z0-9_-]{11}", s.strip()) else ""


@router.post("/uploads")
def add_upload(body: UploadIn):
    vid = _video_id(body.youtube_url)
    with db.tx() as c:
        cur = c.execute("INSERT INTO uploads(analysis_id, youtube_video_id, title, keyword, tags, published_at, notes) "
                        "VALUES(?,?,?,?,?,?,?)", (body.analysis_id, vid, body.title, body.keyword, body.tags,
                                                  body.published_at, body.notes))
    return {"id": cur.lastrowid, "youtube_video_id": vid}


@router.get("/uploads")
def list_uploads():
    with db.tx() as c:
        rows = c.execute("""
            SELECT u.*, s.views, s.likes, s.comments, s.fetched_at
            FROM uploads u LEFT JOIN upload_stats s ON s.id = (
                SELECT id FROM upload_stats WHERE upload_id = u.id ORDER BY id DESC LIMIT 1)
            ORDER BY u.id DESC""").fetchall()
    return [dict(r) for r in rows]


@router.delete("/uploads/{uid}")
def delete_upload(uid: int):
    with db.tx() as c:
        c.execute("DELETE FROM uploads WHERE id=?", (uid,))
    return {"ok": True}


@router.post("/uploads/refresh")
async def refresh_uploads():
    """1 YouTube unit per 50 videos."""
    if not youtube.available():
        raise HTTPException(400, "Kein YouTube-API-Key eingetragen")
    with db.tx() as c:
        rows = c.execute("SELECT id, youtube_video_id FROM uploads WHERE youtube_video_id != ''").fetchall()
    ids = [r["youtube_video_id"] for r in rows]
    if not ids:
        return {"updated": 0}
    try:
        vids = {v["id"]: v for v in await youtube.videos(ids)}
    except youtube.QuotaExceeded as e:
        raise HTTPException(429, str(e))
    n = 0
    with db.tx() as c:
        for r in rows:
            v = vids.get(r["youtube_video_id"])
            if v:
                c.execute("INSERT INTO upload_stats(upload_id, views, likes, comments) VALUES(?,?,?,?)",
                          (r["id"], v["views"], v["likes"], v["comments"]))
                if not c.execute("SELECT title FROM uploads WHERE id=?", (r["id"],)).fetchone()["title"]:
                    c.execute("UPDATE uploads SET title=?, published_at=? WHERE id=?", (v["title"], v["published"], r["id"]))
                n += 1
    return {"updated": n}


@router.post("/channel/sync")
async def sync_channel():
    """Fetch own subscriber count from MY_CHANNEL_ID (1 unit)."""
    cid = settings.get("MY_CHANNEL_ID").strip()
    if not cid or not youtube.available():
        raise HTTPException(400, "Kanal-ID und YouTube-Key in den Einstellungen eintragen")
    ch = await youtube.channels([cid])
    if cid not in ch:
        raise HTTPException(404, "Kanal nicht gefunden")
    settings.save({"MY_SUBSCRIBERS": str(ch[cid]["subs"])})
    return ch[cid]
