"""Core endpoints: module list, settings, health, quota."""
from __future__ import annotations

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

from . import cache, db, jobs, registry
from .config import DEFAULTS, SECRET_KEYS, mask, settings
from .net import client

router = APIRouter(prefix="/api/core", tags=["core"])

CORE_SETTINGS = [
    {"key": "YOUTUBE_API_KEY", "label": "YouTube Data API v3 Key", "secret": True,
     "help": "Kostenlos: console.cloud.google.com → Projekt → 'YouTube Data API v3' aktivieren → Anmeldedaten → API-Schlüssel",
     "link": "https://console.cloud.google.com/apis/library/youtube.googleapis.com"},
    {"key": "YT_DAILY_BUDGET", "label": "Max. YouTube-Einheiten pro Tag", "secret": False,
     "help": "Normale API-Aufrufe (Video-/Kanal-Statistiken). Google erlaubt 10.000 pro Tag."},
    {"key": "YT_SEARCH_BUDGET", "label": "Max. YouTube-Suchen pro Tag", "secret": False,
     "help": "Seit 2026 hat search.list ein eigenes Limit von ca. 100 Aufrufen/Tag."},
    {"key": "LASTFM_API_KEY", "label": "Last.fm API Key (optional)", "secret": True,
     "help": "Kostenlos, verbessert ähnliche Künstler & Hörerzahlen", "link": "https://www.last.fm/api/account/create"},
    {"key": "ANTHROPIC_API_KEY", "label": "Claude / Anthropic API Key (optional)", "secret": True,
     "help": "Kostenpflichtig (wenige Cent pro Analyse). Schreibt zusätzliche Titel & Beschreibungen.",
     "link": "https://console.anthropic.com/"},
    {"key": "ANTHROPIC_MODEL", "label": "Claude-Modell", "secret": False,
     "help": "claude-sonnet-5-5 (günstig) oder claude-opus-5-5 (beste Qualität)"},
    {"key": "REGION_CODE", "label": "Region für YouTube-Suchen", "secret": False,
     "help": "z.B. US (größter Type-Beat-Markt), DE, GB"},
    {"key": "LAN_ACCESS", "label": "Zugriff vom Handy im WLAN (ja/nein)", "secret": False,
     "help": "ja + PIN setzen, dann Dashboard neu starten. Unten erscheint ein QR-Code zum Scannen."},
    {"key": "ACCESS_PIN", "label": "PIN für Handy-Zugriff", "secret": True, "help": "mind. 6 Ziffern"},
    {"key": "NTFY_TOPIC", "label": "Push aufs Handy: ntfy-Thema", "secret": True,
     "help": "App „ntfy“ installieren, ein langes zufälliges Thema abonnieren (z.B. dashboard-x7k2p9q4) und hier eintragen. Jeden Morgen kommt eine Übersicht.",
     "link": "https://ntfy.sh/"},
    {"key": "DIGEST_HOUR", "label": "Uhrzeit für den Morgen-Push", "secret": False, "help": "Stunde, z.B. 8"},
    {"key": "PRODUCER_NAME", "label": "Dein Produzentenname", "secret": False, "help": "für 'prod. by ...'"},
    {"key": "STORE_LINK", "label": "Store-Link (BeatStars/Airbit)", "secret": False, "help": "kommt in die erste Beschreibungszeile"},
    {"key": "INSTAGRAM", "label": "Instagram / Kontakt", "secret": False, "help": "z.B. @deinname oder E-Mail"},
]


def all_setting_defs() -> list[dict]:
    seen, out = set(), []
    for s in CORE_SETTINGS + registry.module_settings():
        if s["key"] not in seen:
            seen.add(s["key"])
            out.append(s)
    return out


@router.get("/modules")
def list_modules():
    return registry.public_list()


class ModuleToggle(BaseModel):
    enabled: bool


@router.put("/modules/{mod_id}")
def toggle_module(mod_id: str, body: ModuleToggle):
    if not any(m["id"] == mod_id for m in registry.LOADED):
        raise HTTPException(404)
    with db.tx() as c:
        c.execute("INSERT OR REPLACE INTO module_state(id, enabled) VALUES(?,?)", (mod_id, int(body.enabled)))
    return {"ok": True}


@router.get("/health")
def health():
    return {
        "youtube": settings.has("YOUTUBE_API_KEY"),
        "lastfm": settings.has("LASTFM_API_KEY"),
        "anthropic": settings.has("ANTHROPIC_API_KEY"),
        "suggest": True,
        "deezer": True,
    }


@router.get("/settings")
def get_settings():
    out = []
    for s in all_setting_defs():
        val = settings.get(s["key"], DEFAULTS.get(s["key"], ""))
        is_secret = s.get("secret") or s["key"] in SECRET_KEYS
        out.append({**s, "value": mask(val) if is_secret else val, "is_set": bool(val)})
    return out


class SettingsUpdate(BaseModel):
    values: dict[str, str]


@router.put("/settings")
def put_settings(body: SettingsUpdate):
    allowed = {s["key"] for s in all_setting_defs()}
    updates = {}
    for k, v in body.values.items():
        if k not in allowed:
            continue
        if "••••" in v:  # masked value sent back unchanged
            continue
        updates[k] = v.replace("\n", "").strip()
    settings.save(updates)
    return {"ok": True, "saved": sorted(updates)}


@router.post("/settings/test/{service}")
async def test_service(service: str):
    c = client()
    try:
        if service == "youtube":
            if not settings.has("YOUTUBE_API_KEY"):
                return {"ok": False, "message": "Kein Key eingetragen"}
            r = await c.get("https://www.googleapis.com/youtube/v3/videos",
                            params={"part": "id", "id": "dQw4w9WgXcQ", "key": settings.get("YOUTUBE_API_KEY")})
            cache.log_units("youtube", 1, "videos.list(test)")
        elif service == "lastfm":
            if not settings.has("LASTFM_API_KEY"):
                return {"ok": False, "message": "Kein Key eingetragen"}
            r = await c.get("https://ws.audioscrobbler.com/2.0/", params={
                "method": "artist.getInfo", "artist": "Drake", "api_key": settings.get("LASTFM_API_KEY"), "format": "json"})
        elif service == "anthropic":
            if not settings.has("ANTHROPIC_API_KEY"):
                return {"ok": False, "message": "Kein Key eingetragen"}
            r = await c.get("https://api.anthropic.com/v1/models", headers={
                "x-api-key": settings.get("ANTHROPIC_API_KEY"), "anthropic-version": "2023-06-01"})
        elif service == "ntfy":
            from . import digest
            title, lines = digest.build()
            ok = await digest.send("Test: " + title, lines or ["Push funktioniert ✔"])
            return {"ok": ok, "message": "Push gesendet – schau aufs Handy ✔" if ok else "Kein Thema eingetragen oder ntfy.sh nicht erreichbar"}
        elif service == "suggest":
            r = await c.get("https://suggestqueries.google.com/complete/search",
                            params={"client": "firefox", "ds": "yt", "q": "type beat"})
        elif service == "deezer":
            r = await c.get("https://api.deezer.com/search/artist", params={"q": "drake", "limit": 1})
        else:
            raise HTTPException(404)
    except Exception as e:  # network problems
        return {"ok": False, "message": f"Verbindung fehlgeschlagen: {e.__class__.__name__}"}
    ok = r.status_code == 200 and "error" not in r.text[:200].lower()
    return {"ok": ok, "message": "Funktioniert ✔" if ok else f"Fehler (HTTP {r.status_code}): {r.text[:200]}"}


@router.get("/quota")
def quota():
    used = cache.usage_today()
    return {
        "day": cache.quota_day(),
        "youtube_units": used.get("youtube", 0),
        "youtube_budget": int(settings.get("YT_DAILY_BUDGET", "8000") or 8000),
        "youtube_searches": used.get("youtube_search", 0),
        "youtube_search_budget": int(settings.get("YT_SEARCH_BUDGET", "90") or 90),
        "other": {k: v for k, v in used.items() if not k.startswith("youtube")},
    }


@router.get("/jobs")
def list_jobs():
    return jobs.running()


@router.get("/jobs/{job_id}")
def get_job(job_id: str):
    j = jobs.get(job_id)
    if not j:
        raise HTTPException(404, "Job nicht gefunden")
    return j.public()


@router.delete("/jobs/{job_id}")
def cancel_job(job_id: str):
    j = jobs.get(job_id)
    if j:
        j.cancelled = True
    return {"ok": True}


@router.get("/lan")
def lan_info():
    from . import lan
    from .__main__ import lan_enabled
    enabled = lan_enabled() and settings.has("ACCESS_PIN")
    url = f"http://{lan.local_ip()}:{settings.get('PORT') or 8765}"
    qr = ""
    if enabled:
        try:
            import segno
            qr = segno.make(url, error="m").svg_inline(scale=5, dark="#1d1d1f")
        except ImportError:
            pass
    return {"enabled": enabled, "url": url, "qr": qr}
