"""Daily morning digest as push notification via ntfy.sh (free app for iOS/Android, no account needed).

The topic name works like a password - anyone who knows it can read the messages, so use a random one.
"""
from __future__ import annotations

import asyncio
import logging
from datetime import date, datetime, timedelta

from . import db
from .config import settings
from .net import client

log = logging.getLogger("beat-dashboard.digest")


def _has(c, table: str) -> bool:
    return bool(c.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (table,)).fetchone())


def build() -> tuple[str, list[str]]:
    today = date.today().isoformat()
    tomorrow = (date.today() + timedelta(days=1)).isoformat()
    lines = []
    with db.tx() as c:
        if _has(c, "leads"):
            due = c.execute("SELECT COUNT(*) FROM leads WHERE archived=0 AND next_action_date<=? AND status NOT IN "
                            "('gebucht','geliefert','retainer','kein_interesse')", (today,)).fetchone()[0]
            if due:
                lines.append(f"⏰ {due} Follow-ups fällig")
        if _has(c, "artist_releases"):
            rel = c.execute("SELECT COUNT(*) FROM artist_releases WHERE date(seen_at)>=date('now','-1 day') AND release_date>=date('now','-14 day')").fetchone()[0]
            if rel:
                lines.append(f"🎵 {rel} neue Releases deiner Artists – DM-Moment!")
        if _has(c, "shoots"):
            for s in c.execute("SELECT title, date, call_time FROM shoots WHERE status='geplant' AND date IN (?,?) ORDER BY date", (today, tomorrow)):
                lines.append(f"🎬 {'Heute' if s['date'] == today else 'Morgen'}: {s['title']}{' ab ' + s['call_time'] if s['call_time'] else ''}")
        if _has(c, "documents"):
            od = c.execute("SELECT COUNT(*), COALESCE(SUM(total),0) FROM documents WHERE type='rechnung' AND status='gesendet' AND due_date<?", (today,)).fetchone()
            if od[0]:
                lines.append(f"🧾 {od[0]} Rechnung(en) überfällig ({od[1]:.0f} €)")
    title = "Dein Tag" if lines else "Alles erledigt"
    return title, lines


async def send(title: str, lines: list[str]) -> bool:
    topic = (settings.get("NTFY_TOPIC") or "").strip()
    if not topic:
        return False
    try:
        r = await client().post("https://ntfy.sh/", json={"topic": topic, "title": title, "tags": ["calendar"],
                                                          "message": "\n".join(lines) or "Keine offenen Aufgaben 🎉"}, timeout=10)
        return r.status_code == 200
    except Exception:
        return False


async def loop():
    """Send once per day after DIGEST_HOUR (default 8) while the dashboard is running."""
    while True:
        try:
            hour = int(settings.get("DIGEST_HOUR", "8") or 8)
            now = datetime.now()
            with db.tx() as c:
                last = c.execute("SELECT value FROM settings_meta WHERE key='digest_last'").fetchone()
            if settings.get("NTFY_TOPIC") and now.hour >= hour and (not last or last[0] != date.today().isoformat()):
                title, lines = build()
                if lines and await send(title, lines):
                    log.info("Tages-Push gesendet")
                with db.tx() as c:
                    c.execute("INSERT OR REPLACE INTO settings_meta(key, value) VALUES('digest_last', ?)", (date.today().isoformat(),))
        except Exception:
            log.exception("Digest fehlgeschlagen")
        await asyncio.sleep(600)
