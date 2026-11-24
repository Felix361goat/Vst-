"""Optional: Claude writes extra titles + a polished description. Falls back silently."""
from __future__ import annotations

import json
import re

from ....config import settings
from ....net import client

URL = "https://api.anthropic.com/v1/messages"

SYSTEM = """You are an expert YouTube SEO assistant for a type-beat producer channel.
Rules: max 2 artists joined with " x ", "Type Beat" exactly once, beat name in quotes, title <= 80 chars with
"<Artist> Type Beat" in the first 45 chars, no "ft.", "feat", "leak", "official", "unreleased", no real song titles,
no emojis in titles. Descriptions: store link + BPM/key in line 1, licensing in line 2, 1-3 natural sentences
using the main keyword (no keyword stuffing), exactly 3 hashtags at the end.
Answer ONLY with JSON: {"titles": [5 strings], "description": string, "extra_tags": [up to 8 strings],
"thumbnail_text": [3 short options, 1-3 words each], "tip": string (one German sentence of strategy advice)}"""


def available() -> bool:
    return settings.has("ANTHROPIC_API_KEY")


async def polish(facts: dict) -> dict | None:
    if not available():
        return None
    body = {
        "model": settings.get("ANTHROPIC_MODEL", "claude-sonnet-5-5") or "claude-sonnet-5-5",
        "max_tokens": 1500,
        "system": SYSTEM,
        "messages": [{"role": "user", "content": "Beat facts and research data:\n" + json.dumps(facts, ensure_ascii=False)}],
    }
    try:
        r = await client().post(URL, json=body, timeout=60, headers={
            "x-api-key": settings.get("ANTHROPIC_API_KEY"), "anthropic-version": "2023-06-01",
            "content-type": "application/json"})
        if r.status_code != 200:
            return {"error": f"Claude-API Fehler {r.status_code}: {r.text[:200]}"}
        text = "".join(b.get("text", "") for b in r.json().get("content", []) if b.get("type") == "text")
        m = re.search(r"\{.*\}", text, re.S)
        data = json.loads(m.group(0)) if m else None
        if not isinstance(data, dict):
            return {"error": "Claude hat kein gültiges JSON geliefert"}
        return data
    except Exception as e:
        return {"error": f"Claude nicht erreichbar: {e.__class__.__name__}"}
