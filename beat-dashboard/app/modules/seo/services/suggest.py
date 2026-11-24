"""YouTube autocomplete (unofficial, no key). Real search phrases people type."""
from __future__ import annotations

import asyncio
import json
import time

from .... import cache
from ....net import client

URL = "https://suggestqueries.google.com/complete/search"
TTL = 3 * 24 * 3600
_lock = asyncio.Lock()
_last = 0.0


async def suggest(q: str, hl: str = "en", gl: str = "US") -> list[str]:
    q = " ".join(q.lower().split())
    key = cache.make_key("suggest", "yt", {"q": q, "hl": hl, "gl": gl})
    hit = cache.get(key)
    if hit is not None:
        return hit
    global _last
    async with _lock:  # be polite: ~4 requests/second max
        wait = 0.25 - (time.time() - _last)
        if wait > 0:
            await asyncio.sleep(wait)
        _last = time.time()
        try:
            r = await client().get(URL, params={"client": "firefox", "ds": "yt", "q": q, "hl": hl, "gl": gl})
            data = json.loads(r.text)
            out = [s for s in data[1] if isinstance(s, str)]
        except Exception:
            return []
    cache.put(key, "suggest", out, TTL)
    return out


async def demand_probe(phrase: str) -> dict:
    """How early does `phrase` appear in autocomplete while typing it?

    Typing "ken carson ty" and seeing "ken carson type beat" at rank 1 means lots of people search it.
    """
    phrase = phrase.lower().strip()
    base = phrase.replace(" type beat", "").strip()
    # (prefix, weight): appearing while only the (partial) name is typed = strong demand.
    # Typing "<name> ty" almost always shows it, so that only counts a little.
    probes = []
    if len(base) > 6:
        probes.append((base[: max(4, int(len(base) * 0.6))], 1.0))
    probes += [(base, 0.9), (base + " t", 0.6), (base + " ty", 0.35)]
    best, found_at = 0.0, None
    for p, w in probes:
        sugg = await suggest(p)
        hits = [i for i, s in enumerate(sugg) if s == phrase or s.startswith(phrase + " ")]
        if hits:
            rank = hits[0]
            score = (1 - rank / 12) * w
            if score > best:
                best, found_at = score, {"prefix": p, "rank": rank + 1}
    expansions = await suggest(phrase + " ")
    # many distinct long-tail variants = an active keyword
    variety = len([e for e in expansions if e.startswith(phrase)]) / 10
    best = min(1.0, best * (0.8 + 0.2 * variety))
    return {"phrase": phrase, "score": round(max(0.0, best), 3), "found_at": found_at,
            "expansions": [e for e in expansions if e != phrase][:10]}


async def long_tail(seed: str, letters: str = "abcdefghijklmnoprstuvwy") -> list[str]:
    """seed + a..z → long-tail keyword list (cached, ~25 requests the first time)."""
    out: list[str] = []
    for s in await suggest(seed):
        if s not in out:
            out.append(s)
    for ch in letters:
        for s in await suggest(f"{seed} {ch}"):
            if s not in out:
                out.append(s)
    return out
