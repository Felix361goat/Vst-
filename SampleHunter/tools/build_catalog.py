#!/usr/bin/env python3
"""Turn hunt results into a catalogue that ships with the app.

    python tools/build_catalog.py LIBRARY_DIR [LIBRARY_DIR ...] --out catalog/catalog-001.json.gz

Takes libraries hunted with --store file, applies the strict quality bar
again on the actual cut audio (crackle, clicks), adds the alignment anchor,
drops the audio itself, packs the embeddings, and writes one gzipped JSON:
pointers only, a few kilobytes per sample.
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HERE)

from hunter import analyze, cloud  # noqa: E402
from hunter.library import pack_emb  # noqa: E402

KEEP = ["id", "kind", "name", "term", "title", "creator", "year", "source_url", "source_key",
        "license", "license_label", "license_url", "trust", "trust_label", "origin", "downloads",
        "bpm", "beats", "duration", "key", "score", "freak", "weird", "grain", "clicks",
        "features", "centroid", "tags", "peaks", "family", "stream", "source_start", "added"]


def write_index(folder: str) -> None:
    """index.json lists every catalogue file with its hash, so the app can
    fetch exactly what changed."""
    import hashlib
    import time
    files = []
    for name in sorted(os.listdir(folder)):
        if name.endswith(".json.gz"):
            with open(os.path.join(folder, name), "rb") as fh:
                body = fh.read()
            with gzip.open(os.path.join(folder, name), "rt", encoding="utf-8") as fh:
                count = len(json.load(fh).get("samples", []))
            files.append({"name": name, "sha256": hashlib.sha256(body).hexdigest(), "count": count})
    with open(os.path.join(folder, "index.json"), "w", encoding="utf-8") as fh:
        json.dump({"updated": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "files": files,
                   "total": sum(f["count"] for f in files)}, fh, indent=1)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("libraries", nargs="+")
    ap.add_argument("--out", required=True)
    ap.add_argument("--max-grain", type=float, default=40)
    ap.add_argument("--max-clicks", type=float, default=30)
    ap.add_argument("--min-score", type=float, default=60)
    args = ap.parse_args()

    seen, out, dropped = set(), [], {}
    for root in args.libraries:
        try:
            with open(os.path.join(root, "library.json"), encoding="utf-8") as fh:
                samples = json.load(fh)["samples"]
        except (OSError, ValueError):
            continue
        for s in samples:
            def drop(why):
                dropped[why] = dropped.get(why, 0) + 1
            st = s.get("stream")
            if not st or not st.get("url"):
                drop("kein Stream")
                continue
            # The same cut found twice (parallel hunts) is kept once.
            dedupe = (st["url"], round(st["start"], 1))
            if dedupe in seen:
                drop("doppelt")
                continue
            if s.get("score", 0) < args.min_score:
                drop("Score")
                continue
            if s.get("grain") is not None and s["grain"] > args.max_grain:
                drop("Grain")
                continue
            path = os.path.join(root, s["file"]) if s.get("file") else os.path.join(root, "_stream", s["id"] + ".mp3")
            if not os.path.exists(path):
                drop("Datei fehlt")
                continue
            audio = analyze.decode(path, sr=44100, channels=2, duration=None)
            cps = analyze.clicks(audio, 44100)
            if cps > args.max_clicks:
                drop("Knackser")
                continue
            s["clicks"] = round(cps, 1)
            st["anchor"] = cloud.make_anchor(audio)
            st.setdefault("kind", s.get("kind", "loop"))
            if not s.get("source_key"):
                s["source_key"] = "ia:" + s["source_url"].rstrip("/").split("/")[-1]
            rec = {k: s[k] for k in KEEP if k in s}
            if s.get("emb"):
                rec["emb_q"] = pack_emb(s["emb"])
            out.append(rec)
            seen.add(dedupe)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with gzip.open(args.out, "wt", encoding="utf-8") as fh:
        json.dump({"version": 1, "samples": out}, fh, ensure_ascii=False, separators=(",", ":"))
    print("%d Samples im Katalog (%s), %d KB" % (len(out), args.out, os.path.getsize(args.out) // 1024))
    write_index(os.path.dirname(os.path.abspath(args.out)))
    for why, n in sorted(dropped.items(), key=lambda x: -x[1]):
        print("  aussortiert, %s: %d" % (why, n))
    return 0


if __name__ == "__main__":
    sys.exit(main())
