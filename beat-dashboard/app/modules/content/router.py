"""/api/content/* - content calendar for the user's own channels."""
from __future__ import annotations

from datetime import date, timedelta

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, field_validator

from ... import db
from ...config import settings

router = APIRouter()
PLATFORMS = {"yt_beat": "YouTube Beat", "yt_short": "YouTube Short", "ig_reel": "Instagram Reel", "ig_post": "Instagram Post",
             "tiktok": "TikTok"}
STATUSES = ["idee", "produziert", "geplant", "gepostet"]
BEST_TIMES = {"yt_beat": "18:00", "yt_short": "17:00", "ig_reel": "19:00", "ig_post": "12:00", "tiktok": "20:00"}


class PostIn(BaseModel):
    date: str
    time: str = ""
    platform: str = "yt_beat"
    title: str = ""
    status: str = "idee"
    link: str = ""
    notes: str = ""
    analysis_id: int | None = None
    shoot_id: int | None = None

    @field_validator("date")
    @classmethod
    def _d(cls, v):
        return date.fromisoformat(v).isoformat()

    @field_validator("platform")
    @classmethod
    def _p(cls, v):
        if v not in PLATFORMS:
            raise ValueError("unbekannte Plattform")
        return v

    @field_validator("status")
    @classmethod
    def _s(cls, v):
        if v not in STATUSES:
            raise ValueError("unbekannter Status")
        return v


def _goal(key: str, default: int) -> int:
    try:
        return int(settings.get(key) or default)
    except ValueError:
        return default


@router.get("/meta")
def meta():
    return {"platforms": PLATFORMS, "statuses": STATUSES, "best_times": BEST_TIMES}


@router.get("/posts")
def posts(start: str, end: str):
    with db.tx() as c:
        rows = c.execute("SELECT * FROM posts WHERE date BETWEEN ? AND ? ORDER BY date, time", (start, end)).fetchall()
    return [dict(r) for r in rows]


@router.post("/posts")
def add_post(p: PostIn):
    if not p.time:
        p.time = BEST_TIMES.get(p.platform, "")
    with db.tx() as c:
        cur = c.execute("INSERT INTO posts(date, time, platform, title, status, link, notes, analysis_id, shoot_id) VALUES(?,?,?,?,?,?,?,?,?)",
                        (p.date, p.time, p.platform, p.title, p.status, p.link, p.notes, p.analysis_id, p.shoot_id))
    return {"id": cur.lastrowid}


@router.put("/posts/{pid}")
def update_post(pid: int, p: PostIn):
    with db.tx() as c:
        if not c.execute("SELECT 1 FROM posts WHERE id=?", (pid,)).fetchone():
            raise HTTPException(404)
        c.execute("UPDATE posts SET date=?, time=?, platform=?, title=?, status=?, link=?, notes=? WHERE id=?",
                  (p.date, p.time, p.platform, p.title, p.status, p.link, p.notes, pid))
    return {"ok": True}


@router.delete("/posts/{pid}")
def del_post(pid: int):
    with db.tx() as c:
        c.execute("DELETE FROM posts WHERE id=?", (pid,))
    return {"ok": True}


@router.get("/week")
def week():
    """Progress of the current week (Mon-Sun) against the goals + streak of weeks that hit the beat goal."""
    today = date.today()
    mon = today - timedelta(days=today.weekday())
    g_beats, g_shorts = _goal("GOAL_BEATS_WEEK", 3), _goal("GOAL_SHORTS_WEEK", 4)

    def counts(start: date):
        with db.tx() as c:
            rows = c.execute("SELECT platform, status, COUNT(*) n FROM posts WHERE date BETWEEN ? AND ? GROUP BY platform, status",
                             (start.isoformat(), (start + timedelta(days=6)).isoformat())).fetchall()
        beats = sum(r["n"] for r in rows if r["platform"] == "yt_beat" and r["status"] == "gepostet")
        shorts = sum(r["n"] for r in rows if r["platform"] != "yt_beat" and r["status"] == "gepostet")
        planned = sum(r["n"] for r in rows if r["status"] != "gepostet")
        return beats, shorts, planned

    beats, shorts, planned = counts(mon)
    streak, wk = 0, mon - timedelta(days=7)
    while streak < 52 and counts(wk)[0] >= g_beats:
        streak += 1
        wk -= timedelta(days=7)
    with db.tx() as c:
        today_posts = [dict(r) for r in c.execute("SELECT * FROM posts WHERE date=? AND status<>'gepostet' ORDER BY time", (today.isoformat(),))]
    return {"beats": beats, "beats_goal": g_beats, "shorts": shorts, "shorts_goal": g_shorts, "open": planned,
            "streak": streak, "today": today_posts}
