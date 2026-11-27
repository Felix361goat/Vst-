"""Release radar: new singles/EPs of tracked artists via the free Deezer API (no key)."""
from __future__ import annotations

import asyncio
import json
import re
from datetime import date, timedelta

from .... import cache, db
from ....net import client

FRESH_DAYS = 45


def _norm(s: str) -> str:
    return re.sub(r"[^a-z0-9]", "", (s or "").lower().replace("$", "s"))


async def _json(url: str, params: dict | None = None, ttl: int = 6 * 3600):
    key = cache.make_key("deezer", url, params or {})
    hit = cache.get(key)
    if hit is not None:
        return hit
    try:
        r = await client().get(url, params=params, timeout=15)
        data = r.json() if r.status_code == 200 else None
    except Exception:
        data = None
    if isinstance(data, dict) and not data.get("error"):
        cache.put(key, "deezer", data, ttl)
        return data
    return None


async def find_deezer_id(name: str) -> dict | None:
    data = await _json("https://api.deezer.com/search/artist", {"q": name, "limit": 10}, ttl=30 * 24 * 3600)
    if not data:
        return None
    exact = [a for a in data.get("data", []) if _norm(a["name"]) == _norm(name)]
    if not exact:
        return None
    a = max(exact, key=lambda a: a.get("nb_fan", 0))
    return {"id": a["id"], "fans": a.get("nb_fan", 0), "link": a.get("link")}


async def scan(job=None) -> dict:
    with db.tx() as c:
        leads = [dict(r) for r in c.execute(
            "SELECT id, name, extra, flags FROM leads WHERE kind='artist' AND archived=0 "
            "AND COALESCE(json_extract(flags,'$.do_not_contact'),0)=0")]
    found, matched = [], 0
    cutoff = (date.today() - timedelta(days=FRESH_DAYS)).isoformat()
    sem = asyncio.Semaphore(5)  # Deezer allows ~50 requests / 5 s

    async def one(lead):
        nonlocal matched
        async with sem:
            extra = json.loads(lead["extra"] or "{}")
            dz = extra.get("deezer_id")
            if dz is None:
                hit = await find_deezer_id(lead["name"])
                dz = hit["id"] if hit else 0
                extra.update(deezer_id=dz, deezer_fans=hit and hit["fans"], deezer=hit and hit["link"])
                with db.tx() as c:
                    c.execute("UPDATE leads SET extra=? WHERE id=?", (json.dumps(extra, ensure_ascii=False), lead["id"]))
            if not dz:
                return
            matched += 1
            data = await _json(f"https://api.deezer.com/artist/{dz}/albums", {"limit": 15})
            if not data:
                return
            with db.tx() as c:
                first_scan = c.execute("SELECT COUNT(*) FROM artist_releases WHERE lead_id=?", (lead["id"],)).fetchone()[0] == 0
                for a in data.get("data", []):
                    cur = c.execute(
                        "INSERT OR IGNORE INTO artist_releases(lead_id, release_id, title, type, release_date, link, cover) VALUES(?,?,?,?,?,?,?)",
                        (lead["id"], str(a["id"]), a.get("title"), a.get("record_type"), a.get("release_date"), a.get("link"), a.get("cover_medium")))
                    is_new = cur.rowcount == 1
                    fresh = (a.get("release_date") or "") >= cutoff
                    if is_new and fresh:
                        found.append({"lead_id": lead["id"], "name": lead["name"], "title": a.get("title"), "date": a.get("release_date")})
                        flags = json.loads(lead["flags"] or "{}")
                        flags["recent_release"] = True
                        from .pipeline import score
                        full = dict(c.execute("SELECT * FROM leads WHERE id=?", (lead["id"],)).fetchone())
                        full.update(flags=flags, extra=extra)
                        c.execute("UPDATE leads SET score=? WHERE id=?", (score(full), lead["id"]))
                        c.execute("UPDATE leads SET flags=?, next_action_date=?, updated_at=datetime('now','localtime') WHERE id=? "
                                  "AND status NOT IN ('gebucht','geliefert','kein_interesse')",
                                  (json.dumps(flags), date.today().isoformat(), lead["id"]))
                        c.execute("INSERT INTO lead_activities(lead_id, type, channel, text) VALUES(?,?,?,?)",
                                  (lead["id"], "release", "deezer", f"🎵 Neuer Release: „{a.get('title')}“ ({a.get('record_type')}, {a.get('release_date')})"))
                    elif is_new and first_scan:
                        pass  # old catalogue, just remember it
            if job:
                job.update(done=job.done + 1, message=f"{lead['name']}")

    if job:
        job.update(done=0, total=len(leads), message="Deezer abfragen …")
    await asyncio.gather(*(one(l) for l in leads))
    with db.tx() as c:
        c.execute("INSERT OR REPLACE INTO settings_meta(key, value) VALUES('radar_last_run', ?)", (date.today().isoformat(),))
    return {"artists": len(leads), "on_deezer": matched, "new_releases": len(found), "releases": found[:30]}


def recent(days: int = 60) -> list[dict]:
    cutoff = (date.today() - timedelta(days=days)).isoformat()
    with db.tx() as c:
        rows = c.execute(
            "SELECT r.*, l.name, l.instagram, l.status FROM artist_releases r JOIN leads l ON l.id=r.lead_id "
            "WHERE r.release_date>=? AND l.archived=0 ORDER BY r.release_date DESC LIMIT 40", (cutoff,)).fetchall()
    return [dict(r) for r in rows]


def last_run() -> str | None:
    with db.tx() as c:
        r = c.execute("SELECT value FROM settings_meta WHERE key='radar_last_run'").fetchone()
    return r[0] if r else None
