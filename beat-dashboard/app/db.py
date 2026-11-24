"""Tiny SQLite helper. One file: data/dashboard.db. No ORM on purpose."""
from __future__ import annotations

import json
import sqlite3
from contextlib import contextmanager
from pathlib import Path

from .config import DATA_DIR

DB_PATH = DATA_DIR / "dashboard.db"

CORE_SCHEMA = """
CREATE TABLE IF NOT EXISTS settings_meta(key TEXT PRIMARY KEY, value TEXT);
CREATE TABLE IF NOT EXISTS module_state(id TEXT PRIMARY KEY, enabled INTEGER NOT NULL DEFAULT 1);
CREATE TABLE IF NOT EXISTS api_cache(
    key TEXT PRIMARY KEY, source TEXT, payload TEXT, created_at REAL, expires_at REAL);
CREATE TABLE IF NOT EXISTS quota_log(
    id INTEGER PRIMARY KEY AUTOINCREMENT, day TEXT, source TEXT, units INTEGER, endpoint TEXT, ts REAL);
CREATE INDEX IF NOT EXISTS quota_day ON quota_log(day, source);
"""


def connect() -> sqlite3.Connection:
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA foreign_keys=ON")
    return conn


@contextmanager
def tx():
    conn = connect()
    try:
        yield conn
        conn.commit()
    finally:
        conn.close()


def run_script(sql: str) -> None:
    with tx() as c:
        c.executescript(sql)


def init_core() -> None:
    run_script(CORE_SCHEMA)


def run_module_schema(path: Path) -> None:
    if path.exists():
        run_script(path.read_text(encoding="utf-8"))


def row_to_dict(row: sqlite3.Row | None, json_fields: tuple[str, ...] = ()) -> dict | None:
    if row is None:
        return None
    d = dict(row)
    for f in json_fields:
        if d.get(f):
            try:
                d[f] = json.loads(d[f])
            except (TypeError, ValueError):
                pass
    return d
