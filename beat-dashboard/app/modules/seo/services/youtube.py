"""YouTube Data API v3 - competitor research with strict quota protection.

Quota (2026): search.list has its own bucket (~100 calls/day). videos.list / channels.list
cost 1 unit each (50 ids per call) from the 10,000/day pool. Everything is cached.
"""
from __future__ import annotations

import statistics
from datetime import datetime, timedelta, timezone

from .... import cache
from ....config import settings
from ....net import client

API = "https://www.googleapis.com/youtube/v3"
SEARCH_TTL = 2 * 24 * 3600
STATS_TTL = 24 * 3600


class QuotaExceeded(Exception):
    pass


class YouTubeError(Exception):
    pass


def available() -> bool:
    return settings.has("YOUTUBE_API_KEY")


def searches_left() -> int:
    return max(0, int(settings.get("YT_SEARCH_BUDGET", "90") or 90) - cache.units_today("youtube_search"))


def units_left() -> int:
    return max(0, int(settings.get("YT_DAILY_BUDGET", "8000") or 8000) - cache.units_today("youtube"))


async def _get(endpoint: str, params: dict) -> dict:
    r = await client().get(f"{API}/{endpoint}", params={**params, "key": settings.get("YOUTUBE_API_KEY")})
    data = r.json()
    if r.status_code != 200:
        msg = data.get("error", {}).get("message", r.text[:200])
        if r.status_code == 403 and "quota" in msg.lower():
            raise QuotaExceeded(msg)
        raise YouTubeError(msg)
    return data


def _iso(dt: datetime) -> str:
    return dt.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


async def search(q: str, days: int | None = None, order: str = "relevance", max_results: int = 25) -> dict | None:
    """Returns {'total': int, 'ids': [...]} or None if no key / no budget."""
    if not available():
        return None
    region = settings.get("REGION_CODE", "US") or "US"
    params = {"part": "id", "q": q, "type": "video", "videoCategoryId": "10", "order": order,
              "maxResults": max_results, "regionCode": region, "relevanceLanguage": "en"}
    if days:
        start = (datetime.now(timezone.utc) - timedelta(days=days)).replace(hour=0, minute=0, second=0)
        params["publishedAfter"] = _iso(start)
    key = cache.make_key("yt_search", "search", params)
    hit = cache.get(key)
    if hit is not None:
        return {**hit, "cached": True}
    if searches_left() <= 0:
        raise QuotaExceeded("Tägliches Such-Budget aufgebraucht")
    data = await _get("search", params)
    cache.log_units("youtube_search", 1, "search.list")
    out = {"total": data.get("pageInfo", {}).get("totalResults", 0),
           "ids": [i["id"]["videoId"] for i in data.get("items", []) if i.get("id", {}).get("videoId")]}
    cache.put(key, "yt_search", out, SEARCH_TTL)
    return {**out, "cached": False}


async def _batched(endpoint: str, part: str, ids: list[str], source: str) -> dict[str, dict]:
    result: dict[str, dict] = {}
    missing = []
    for i in ids:
        hit = cache.get(cache.make_key(source, endpoint, {"id": i}))
        if hit is not None:
            result[i] = hit
        else:
            missing.append(i)
    for start in range(0, len(missing), 50):
        chunk = missing[start:start + 50]
        if units_left() <= 0:
            raise QuotaExceeded("Tägliches YouTube-Budget aufgebraucht")
        data = await _get(endpoint, {"part": part, "id": ",".join(chunk), "maxResults": 50})
        cache.log_units("youtube", 1, f"{endpoint}.list")
        for item in data.get("items", []):
            result[item["id"]] = item
            cache.put(cache.make_key(source, endpoint, {"id": item["id"]}), source, item, STATS_TTL)
    return result


async def videos(ids: list[str]) -> list[dict]:
    raw = await _batched("videos", "snippet,statistics,contentDetails", ids, "yt_video")
    out = []
    for i in ids:
        v = raw.get(i)
        if not v:
            continue
        sn, st = v.get("snippet", {}), v.get("statistics", {})
        out.append({
            "id": i, "title": sn.get("title", ""), "channel_id": sn.get("channelId"), "channel": sn.get("channelTitle"),
            "published": sn.get("publishedAt"), "tags": sn.get("tags", []), "description": sn.get("description", "")[:600],
            "views": int(st.get("viewCount", 0)), "likes": int(st.get("likeCount", 0) or 0),
            "comments": int(st.get("commentCount", 0) or 0),
            "thumb": sn.get("thumbnails", {}).get("medium", {}).get("url"),
        })
    return out


async def channels(ids: list[str]) -> dict[str, dict]:
    raw = await _batched("channels", "statistics", list(dict.fromkeys(i for i in ids if i)), "yt_channel")
    out = {}
    for cid, c in raw.items():
        st = c.get("statistics", {})
        out[cid] = {"subs": int(st.get("subscriberCount", 0) or 0), "videos": int(st.get("videoCount", 0) or 0),
                    "hidden": st.get("hiddenSubscriberCount", False)}
    return out


def _age_days(published: str | None) -> float:
    if not published:
        return 9999
    dt = datetime.fromisoformat(published.replace("Z", "+00:00"))
    return (datetime.now(timezone.utc) - dt).total_seconds() / 86400


async def niche_report(phrase: str) -> dict | None:
    """Everything we need to judge one keyword. Costs 2 searches + ~2 units (cached 2 days)."""
    if not available():
        return None
    top = await search(phrase, order="relevance", max_results=25)
    recent = await search(phrase, days=30, order="relevance", max_results=50)
    vids = await videos(top["ids"][:25])
    chans = await channels([v["channel_id"] for v in vids])
    for v in vids:
        c = chans.get(v["channel_id"], {})
        v["channel_subs"] = c.get("subs", 0)
        v["age_days"] = round(_age_days(v["published"]), 1)
        v["exact_in_title"] = phrase.lower() in v["title"].lower()
    views = [v["views"] for v in vids] or [0]
    subs = [v["channel_subs"] for v in vids[:10]] or [0]
    fresh = [v for v in vids if v["age_days"] <= 30]
    # outliers: views at least 3x the channel's subscriber count (proxy for channel median) and < 60 days old
    outliers = [v for v in vids if v["age_days"] <= 60 and v["views"] >= 3 * max(1000, v["channel_subs"])]
    small_in_top10 = sum(1 for s in subs if s < 10_000)
    tag_counts: dict[str, int] = {}
    for v in vids:
        for t in v["tags"]:
            t = t.lower().strip()
            tag_counts[t] = tag_counts.get(t, 0) + 1
    hours = [0] * 24
    weekdays = [0] * 7
    for v in vids:
        if v["published"]:
            dt = datetime.fromisoformat(v["published"].replace("Z", "+00:00"))
            hours[dt.hour] += 1
            weekdays[dt.weekday()] += 1
    return {
        "phrase": phrase,
        "total_results": top["total"],
        "uploads_30d": len(recent["ids"]) if len(recent["ids"]) < 50 else recent["total"],
        "uploads_30d_capped": len(recent["ids"]) >= 50,
        "median_views": int(statistics.median(views)),
        "median_subs_top10": int(statistics.median(subs)),
        "small_channels_top10": small_in_top10,
        "fresh_share": round(len(fresh) / max(1, len(vids)), 2),
        "exact_title_share": round(sum(v["exact_in_title"] for v in vids) / max(1, len(vids)), 2),
        "outliers": sorted(outliers, key=lambda v: -v["views"])[:6],
        "top_videos": vids[:12],
        "top_tags": sorted(tag_counts.items(), key=lambda kv: -kv[1])[:30],
        "publish_hours_utc": hours,
        "publish_weekdays": weekdays,
        "cached": top.get("cached") and recent.get("cached"),
    }
