#!/usr/bin/env python3
"""Go digging: find unknown recordings, cut out their best riffs, file them.

    python hunt.py                     # 20 new samples
    python hunt.py --count 50
    python hunt.py --terms "kora,tumbi,steel guitar"
    python hunt.py --allow-nc          # also CC BY-NC (non-commercial only)
    python hunt.py --include-claimed   # also uploader-licensed Archive items
"""

from __future__ import annotations

import argparse
import hashlib
import os
import random
import re
import sys
import time
import traceback
import urllib.error
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from hunter import analyze, sources  # noqa: E402
from hunter.library import Library, Taste  # noqa: E402


def log(msg: str) -> None:
    print(time.strftime("%H:%M:%S "), msg, flush=True)


def slug(text: str, n: int = 32) -> str:
    text = re.sub(r"[^A-Za-z0-9]+", "-", text).strip("-")
    return text[:n].strip("-") or "x"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--count", type=int, default=20, help="how many new samples to find")
    ap.add_argument("--terms", default="", help="comma-separated search terms (default: all, weighted by your taste)")
    ap.add_argument("--allow-nc", action="store_true", help="include CC BY-NC (non-commercial) material")
    ap.add_argument("--include-claimed", action="store_true",
                    help="also use Archive items whose licence only rests on the uploader's word")
    ap.add_argument("--experiment", action="store_true",
                    help="also dig obscure Archive uploads with unclear licence (to play with; clear before release)")
    ap.add_argument("--kinds", default="loop,phrase",
                    help="what to cut: loop (riffs on the beat), phrase (free one-shot lines), or both")
    ap.add_argument("--min-score", type=float, default=58.0, help="minimum Freak score, 0-100")
    ap.add_argument("--per-track", type=int, default=2, help="max samples cut from one recording")
    ap.add_argument("--library", default=os.path.join(HERE, "library"))
    ap.add_argument("--max-minutes", type=float, default=0, help="stop after this long (0 = no limit)")
    args = ap.parse_args()

    lib = Library(args.library)
    rng = random.Random()
    deadline = time.time() + args.max_minutes * 60 if args.max_minutes else None
    terms = [t.strip() for t in args.terms.split(",") if t.strip()] or list(sources.ALL_TERMS)
    # (source, term) pairs that came back empty this session are not retried.
    dry: set[tuple[str, str]] = set()
    found = 0
    misses = 0

    log("Suche %d neue Samples …" % args.count)
    while found < args.count and misses < 60:
        if deadline and time.time() > deadline:
            log("Zeitlimit erreicht.")
            break
        data = lib.load()
        taste = Taste(data, lib.ratings())

        # Where to dig this round.
        options = [("commons", 0.40), ("78", 0.35)]
        if not args.terms:
            options.append(("78-random", 0.25))
        if args.include_claimed:
            options.append(("ia", 0.30))
        if args.experiment:
            options.append(("ia-any", 0.60))
        src = rng.choices([o[0] for o in options], weights=[o[1] for o in options])[0]
        term = None
        if src != "78-random":
            pool = [t for t in terms if (src, t) not in dry]
            if not pool:
                misses += 1
                continue
            term = rng.choices(pool, weights=[taste.term_weight[t] for t in pool])[0]

        try:
            if src == "commons":
                cands = sources.search_commons(term, args.allow_nc, rng)
            elif src == "ia":
                cands = sources.search_ia_claimed(term, args.allow_nc, rng)
            elif src == "ia-any":
                cands = sources.search_ia_any(term, args.allow_nc, rng)
            else:
                cands = sources.search_78(term, rng)
        except Exception as exc:  # network hiccups should not end the hunt
            log("Suche %s/%s fehlgeschlagen: %s" % (src, term, exc))
            misses += 1
            time.sleep(5)
            continue
        fresh = [c for c in cands if c.key not in data["processed"]]
        label = {"commons": "Wikimedia", "78": "78er", "78-random": "78er-Kiste", "ia": "Archive", "ia-any": "Archive (Experiment)"}[src]
        log("%s · %s: %d neue Aufnahmen" % (label, term or "Zufall", len(fresh)))
        if not cands and term:
            dry.add((src, term))
        if not fresh:
            misses += 1
            continue
        misses = 0
        # Favour the least-played, but keep some chance for everything.
        fresh.sort(key=lambda c: c.downloads)
        picks = fresh[:8]
        rng.shuffle(picks)
        for cand in picks[:2]:
            if found >= args.count:
                break
            found += process(lib, cand, args, taste)
    log("Fertig: %d neue Samples. Bibliothek: %s" % (found, lib.root))
    return 0


def process(lib: Library, cand: sources.Candidate, args, taste: Taste) -> int:
    added = 0
    throttled = False
    try:
        files = sources.files_for(cand, max_files=2)
    except Exception as exc:
        log("  Metadaten fehlgeschlagen (%s): %s" % (cand.identifier, exc))
        files = []
    for f in files:
        ext = os.path.splitext(f["name"])[1] or ".audio"
        tmp = os.path.join(lib.cache_dir, hashlib.md5(f["url"].encode()).hexdigest() + ext)
        try:
            log("  ↓ %s — %s" % (cand.title[:60], f["name"][:50]))
            sources.download(f["url"], tmp)
            kinds = {k.strip() for k in args.kinds.split(",")}
            windows = []
            if "loop" in kinds:
                windows += analyze.find_windows(tmp, max_windows=args.per_track, min_score=args.min_score - 10)
            if "phrase" in kinds:
                windows += analyze.find_phrases(tmp, max_phrases=args.per_track, min_score=args.min_score - 10)
            cut_from, offset = tmp, 0.0
            if windows and f.get("hq_url"):
                hq = tmp + ".hq" + os.path.splitext(urllib.parse.unquote(f["hq_url"]))[1]
                try:
                    log("    Treffer – hole verlustfreies Original")
                    sources.download(f["hq_url"], hq)
                    offset = analyze.align(tmp, hq, windows[0].start)
                    cut_from = hq
                except Exception as exc:
                    log("    Original nicht ladbar (%s), schneide aus MP3" % exc)
            for w in windows:
                sample = to_sample(lib, cand, f, w, cut_from, offset)
                sample["taste"] = taste.bonus(sample)
                if w.score + sample["taste"] < args.min_score:
                    os.remove(os.path.join(lib.root, sample["file"]))
                    continue
                data = lib.load()
                data["samples"].append(sample)
                lib.save(data)
                added += 1
                log("  ✓ %s  Freak %.0f%s  %s  %s" % (
                    sample["name"], w.score,
                    (" (Geschmack %+.0f)" % sample["taste"]) if sample["taste"] else "",
                    ("%d BPM" % round(w.bpm)) if w.kind == "loop" else "Phrase %.1f s" % w.duration, w.key))
            if not windows:
                log("    nichts Brauchbares gefunden")
        except urllib.error.HTTPError as exc:
            log("    Server sagt %d – später nochmal" % exc.code)
            if exc.code == 429:
                throttled = True
                time.sleep(20)
        except Exception as exc:
            log("    Fehler: %s" % exc)
            if os.environ.get("HUNTER_DEBUG"):
                traceback.print_exc()
        finally:
            for leftover in (tmp, tmp + ".hq.flac", tmp + ".hq.wav"):
                if os.path.exists(leftover):
                    os.remove(leftover)
    if not throttled:
        data = lib.load()
        data["processed"][cand.key] = int(time.time())
        lib.save(data)
    return added


def to_sample(lib: Library, cand: sources.Candidate, f: dict, w: analyze.Window, src: str,
              offset: float = 0.0) -> dict:
    sid = hashlib.sha1(("%s|%s|%.2f" % (cand.key, f["name"], w.start)).encode()).hexdigest()[:12]
    bpm = round(w.bpm)
    key = w.key.replace(" ", "").replace("#", "s")
    title = slug(cand.title, 24)
    if title == "x":                       # non-Latin title
        title = slug(cand.identifier, 24)
    shape = ("%dbpm" % bpm) if w.kind == "loop" else "phrase"
    name = "%s_%s_%s_%s_%s" % (slug(cand.term, 16), title, shape, key, sid[:4])
    rel = os.path.join("samples", name + ".wav")
    peaks = analyze.export(src, w, os.path.join(lib.root, rel), offset=offset)
    vintage = bool(cand.year and cand.year < 1960)
    tags = [cand.term] + analyze.describe(w, vintage)
    if cand.downloads < 200:
        tags.append("selten")
    return {
        "id": sid,
        "kind": w.kind,
        "name": name,
        "file": rel.replace(os.sep, "/"),
        "term": cand.term,
        "title": cand.title,
        "creator": cand.creator,
        "year": cand.year,
        "source_url": cand.page_url,
        "source_file": urllib.parse.unquote(os.path.basename(f.get("hq_url") or f["url"])) if src != f.get("url") else f["name"],
        "lossless": src.endswith((".flac", ".wav")),
        "source_start": round(w.start, 3),
        "license": cand.license,
        "license_label": sources.LICENSE_LABELS[cand.license],
        "license_url": cand.license_url,
        "trust": cand.trust,
        "trust_label": sources.TRUST_LABELS[cand.trust],
        "origin": cand.source,
        "downloads": cand.downloads,
        "bpm": round(w.bpm, 2),
        "beats": w.beats,
        "duration": round(w.duration, 3),
        "key": w.key,
        "score": w.score,
        "features": w.features,
        "centroid": round(w.centroid, 1),
        "tags": tags,
        "peaks": peaks,
        "added": time.strftime("%Y-%m-%dT%H:%M:%S"),
    }


if __name__ == "__main__":
    sys.exit(main())
