"""Titles, tags, hashtags, description, pinned comment - rule based (no AI needed)."""
from __future__ import annotations

import re

from .knowledge import ARTIST_VARIANTS, GENRES, title_year

TAG_LIMIT = 500


def tag_cost(tag: str) -> int:
    """YouTube counts quotes around tags with spaces, plus a separating comma."""
    return len(tag) + (2 if " " in tag else 0) + 1


def fit_tags(candidates: list[str], limit: int = TAG_LIMIT, max_tags: int = 25) -> tuple[list[str], int]:
    out, used, seen = [], 0, set()
    for t in candidates:
        t = re.sub(r"[<>,]", "", t).strip().lower()
        if not t or t in seen or len(t) > 100:
            continue
        cost = tag_cost(t)
        if used + cost > limit or len(out) >= max_tags:
            continue
        out.append(t)
        seen.add(t)
        used += cost
    return out, used


def _slug(s: str) -> str:
    return re.sub(r"[^a-z0-9]", "", s.lower())


def build(*, artists: list[str], genre: str, mood: str, beat_name: str, bpm: int | None, key: str | None,
          free: str, producer: str, store_link: str, contact: str, keywords: list[str],
          modifier: str = "", sample_source: str = "original") -> dict:
    g = GENRES.get(genre, {"label": genre or "Trap", "keywords": [], "year_in_title": True})
    genre_word = g["label"].split("/")[0].strip()
    year = title_year()
    a1 = artists[0] if artists else ""
    a2 = artists[1] if len(artists) > 1 else ""
    combo = f"{a1} x {a2}" if a2 else a1
    name = f'"{beat_name.strip()}"' if beat_name.strip() else '"Untitled"'
    tag_free = {"free": "[FREE] ", "free_for_profit": "[FREE FOR PROFIT] ", "none": ""}.get(free, "")
    year_part = f" {year}" if g.get("year_in_title", True) else ""
    mood_genre = f"{mood} {genre_word}".strip()
    bpmkey = " ".join(x for x in [f"{bpm} BPM" if bpm else "", key or ""] if x)
    mod = f" ({modifier})" if modifier else ""

    titles = []
    if a1:
        titles += [
            f"{tag_free}{a1} Type Beat{year_part} - {name}",
            f"{tag_free}{combo} Type Beat{mod} - {name} | {mood_genre}{year_part}",
            f"{tag_free}{a1} Type Beat{mod} - {name} | {mood_genre}",
        ]
    titles += [
        f"{tag_free}{mood_genre} Type Beat - {name}" + (f" | {bpmkey}" if bpmkey else ""),
        f"{tag_free}{mood_genre} Beat{year_part} - {name}" + (f" | prod. {producer}" if producer else ""),
    ]
    titles = [re.sub(r"\s+", " ", t).strip() for t in dict.fromkeys(titles)]
    titles = [{"text": t, "length": len(t), "keyword_end": (t.lower().find("type beat") + 9) if "type beat" in t.lower() else None}
              for t in titles if len(t) <= 100]

    # --- tags in priority order ---
    cands: list[str] = []
    if a1:
        main = f"{a1} type beat"
        cands += [main, f"{main} {year}", f"free {main}"]
        if a2:
            cands += [f"{a1} x {a2} type beat", f"{a2} type beat"]
        for a in artists[2:4]:
            cands.append(f"{a} type beat")
    cands += [k for k in keywords if "type beat" in k][:6]
    gl = genre_word.lower()
    cands += [f"{gl} type beat", f"{gl} beat {year}", f"{mood.lower()} {gl} beat" if mood else ""]
    if modifier:
        cands.append(f"{a1 or gl} type beat {modifier.lower()}")
    cands += [f"{w} beat" for w in g.get("keywords", [])[:3]]
    if bpm:
        cands.append(f"{bpm} bpm")
    if key:
        cands.append(key.lower())
    for a in artists[:2]:
        for v in ARTIST_VARIANTS.get(a.lower(), [])[:1]:
            cands.append(f"{v} type beat")
    if producer:
        cands.append(f"prod {producer}")
    cands += ["type beat", f"type beat {year}"]
    tags, tag_chars = fit_tags([c for c in cands if c])

    # --- hashtags (first 3 show above the title) ---
    hashtags = ["#typebeat"]
    if a1:
        hashtags.append(f"#{_slug(a1)}typebeat")
    hashtags.append(f"#{_slug(genre_word)}beat")
    hashtags = list(dict.fromkeys(hashtags))[:3]

    # --- description ---
    link = store_link or "[DEIN BEATSTARS/AIRBIT-LINK]"
    lic = {
        "free": f'FREE for non-profit use only - credit "(prod. {producer or "DEINNAME"})". Lease for streaming/monetization.',
        "free_for_profit": f'FREE FOR PROFIT - credit "(prod. {producer or "DEINNAME"})" required.',
        "none": "Lease required for any use. Exclusive rights available.",
    }[free if free in ("free", "free_for_profit", "none") else "free"]
    main_kw = f"{a1} type beat" if a1 else f"{genre_word.lower()} type beat"
    sample_line = {"original": "100% original - no samples", "royalty_free": "Contains royalty-free (licensed) samples",
                   "cleared": "Sample cleared"}.get(sample_source, "100% original - no samples")
    lines = [
        f"💰 Purchase / Lease ({bpmkey or 'BPM / Key'}): {link}",
        lic,
        f"📩 Exclusives & customs: {contact or '[KONTAKT]'}",
        "",
        f"{combo or genre_word} Type Beat - {name.strip(chr(34))}",
        f"BPM: {bpm or '-'} | Key: {key or '-'} | Mood: {mood or '-'} | Genre: {g['label']}",
        f"Prod. by {producer or '[DEIN NAME]'} | {sample_line}",
        "",
        f"If you're looking for a {main_kw} with a {(mood or 'dark').lower()} {genre_word.lower()} vibe, this one is for you."
        + (f" Inspired by {combo}, perfect for artists in that lane." if a1 else ""),
        "",
        "Licenses: MP3 Lease | WAV Lease | Trackout (Stems) | Unlimited | Exclusive",
        "",
        "⚠️ This beat is not affiliated with or endorsed by the artists mentioned. Artist names are used for reference only.",
        "",
        " ".join(hashtags),
    ]
    description = "\n".join(lines)
    pinned = f"🔥 Buy / Lease: {link}\nRate this beat 1-10 👇 Who should hop on this?"
    short_title = f"{a1 or mood_genre} Type Beat 🔥 {name}" + (" #shorts" if True else "")
    playlists = [p for p in [f"{a1} Type Beats {year}" if a1 else None, f"{mood_genre} Beats".strip()] if p]
    return {"titles": titles, "tags": tags, "tag_chars": tag_chars, "tags_csv": ", ".join(tags),
            "hashtags": hashtags, "description": description, "pinned_comment": pinned,
            "short_title": short_title, "playlists": playlists}
