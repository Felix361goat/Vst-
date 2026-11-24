"""Rising artists from kworb.net (Spotify monthly listeners, daily change). Scraped once a day."""
from __future__ import annotations

import re
from html import unescape

from .... import cache
from ....net import client

URL = "https://kworb.net/spotify/listeners.html"
TTL = 24 * 3600
ROW = re.compile(r"<tr>(.*?)</tr>", re.S)
CELL = re.compile(r"<td[^>]*>(.*?)</td>", re.S)
TAG = re.compile(r"<[^>]+>")


def _num(s: str) -> int:
    s = s.replace(",", "").strip()
    try:
        return int(s)
    except ValueError:
        return 0


async def rising(limit: int = 40, min_listeners: int = 2_000_000) -> list[dict]:
    key = cache.make_key("kworb", URL, {})
    rows = cache.get(key)
    if rows is None:
        try:
            r = await client().get(URL)
            html = r.text
        except Exception:
            return []
        rows = []
        for m in ROW.finditer(html):
            cells = [unescape(TAG.sub("", c)).strip() for c in CELL.findall(m.group(1))]
            # columns: rank, artist, listeners, daily change, peak, peak listeners
            if len(cells) >= 4 and cells[1]:
                rows.append({"artist": cells[1], "listeners": _num(cells[2]), "daily": _num(cells[3])})
        if rows:
            cache.put(key, "kworb", rows, TTL)
    out = []
    for x in rows:
        if x["listeners"] >= min_listeners and x["daily"] > 0:
            out.append({**x, "growth_pct": round(100 * x["daily"] / max(1, x["listeners"] - x["daily"]), 2)})
    out.sort(key=lambda x: -x["growth_pct"])
    return out[:limit]


async def rising_type_beat_artists(limit: int = 15, scan: int = 80) -> list[dict]:
    """Rising artists that people actually search 'type beat' for (filters out pop/christmas/etc.)."""
    from .suggest import suggest
    out = []
    for x in await rising(limit=scan, min_listeners=1_000_000):
        # only artists where "type beat" is suggested for the bare name -> real producer demand
        sugg = await suggest(x["artist"].lower())
        hits = [s for s in sugg if "type beat" in s]
        if hits:
            out.append({**x, "type_beat_searches": hits[:5]})
            if len(out) >= limit:
                break
    return out
