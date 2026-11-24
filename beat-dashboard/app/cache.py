"""SQLite-backed TTL cache and API quota ledger."""
from __future__ import annotations

import hashlib
import json
import time
from datetime import datetime
from zoneinfo import ZoneInfo

from .db import tx

try:
    PACIFIC = ZoneInfo("America/Los_Angeles")
except Exception:  # Windows without tzdata
    PACIFIC = None


def make_key(source: str, endpoint: str, params: dict) -> str:
    norm = json.dumps({k: params[k] for k in sorted(params)}, sort_keys=True, ensure_ascii=False).lower()
    return hashlib.sha1(f"{source}|{endpoint}|{norm}".encode()).hexdigest()


def get(key: str):
    with tx() as c:
        row = c.execute("SELECT payload, expires_at FROM api_cache WHERE key=?", (key,)).fetchone()
    if row and row["expires_at"] > time.time():
        return json.loads(row["payload"])
    return None


def put(key: str, source: str, payload, ttl_seconds: float) -> None:
    now = time.time()
    with tx() as c:
        c.execute(
            "INSERT OR REPLACE INTO api_cache(key, source, payload, created_at, expires_at) VALUES(?,?,?,?,?)",
            (key, source, json.dumps(payload, ensure_ascii=False), now, now + ttl_seconds),
        )


def purge_expired() -> None:
    with tx() as c:
        c.execute("DELETE FROM api_cache WHERE expires_at < ?", (time.time(),))


def quota_day() -> str:
    """YouTube quota resets at midnight Pacific Time."""
    if PACIFIC:
        return datetime.now(PACIFIC).strftime("%Y-%m-%d")
    return datetime.utcnow().strftime("%Y-%m-%d")


def log_units(source: str, units: int, endpoint: str) -> None:
    with tx() as c:
        c.execute(
            "INSERT INTO quota_log(day, source, units, endpoint, ts) VALUES(?,?,?,?,?)",
            (quota_day(), source, units, endpoint, time.time()),
        )


def units_today(source: str) -> int:
    with tx() as c:
        row = c.execute(
            "SELECT COALESCE(SUM(units),0) AS u FROM quota_log WHERE day=? AND source=?", (quota_day(), source)
        ).fetchone()
    return int(row["u"])


def usage_today() -> dict[str, int]:
    with tx() as c:
        rows = c.execute(
            "SELECT source, SUM(units) AS u FROM quota_log WHERE day=? GROUP BY source", (quota_day(),)
        ).fetchall()
    return {r["source"]: int(r["u"]) for r in rows}
