"""The sample library on disk, and what it has learnt about your taste.

library/
  library.json   every sample with its metadata
  ratings.json   your likes (+1) and dislikes (-1)
  samples/       the WAV files
"""

from __future__ import annotations

import json
import os
import threading
from collections import defaultdict

import numpy as np

from . import analyze

_lock = threading.Lock()


class Library:
    def __init__(self, root: str):
        self.root = os.path.abspath(root)
        self.samples_dir = os.path.join(self.root, "samples")
        self.cache_dir = os.path.join(self.root, "_cache")
        os.makedirs(self.samples_dir, exist_ok=True)
        os.makedirs(self.cache_dir, exist_ok=True)
        self.path = os.path.join(self.root, "library.json")
        self.ratings_path = os.path.join(self.root, "ratings.json")

    # --- storage -------------------------------------------------------
    def _read(self, path: str, default):
        try:
            with open(path, encoding="utf-8") as fh:
                return json.load(fh)
        except (FileNotFoundError, json.JSONDecodeError):
            return default

    def _write(self, path: str, data) -> None:
        tmp = path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            json.dump(data, fh, ensure_ascii=False, indent=1)
        os.replace(tmp, path)

    def load(self) -> dict:
        data = self._read(self.path, {})
        data.setdefault("samples", [])
        data.setdefault("processed", {})
        return data

    def save(self, data: dict) -> None:
        with _lock:
            self._write(self.path, data)

    def ratings(self) -> dict:
        return self._read(self.ratings_path, {})

    def rate(self, sample_id: str, value: int) -> None:
        with _lock:
            r = self._read(self.ratings_path, {})
            if value == 0:
                r.pop(sample_id, None)
            else:
                r[sample_id] = 1 if value > 0 else -1
            self._write(self.ratings_path, r)

    # --- collections: your own folders of samples ----------------------
    @property
    def collections_path(self) -> str:
        return os.path.join(self.root, "collections.json")

    def collections(self) -> dict:
        return self._read(self.collections_path, {})

    def collect(self, sample_id: str, name: str, on: bool) -> None:
        name = name.strip()[:60]
        if not name:
            return
        with _lock:
            c = self._read(self.collections_path, {})
            ids = c.setdefault(name, [])
            if on and sample_id and sample_id not in ids:
                ids.append(sample_id)
            if not on and sample_id in ids:
                ids.remove(sample_id)
            self._write(self.collections_path, c)

    def drop_collection(self, name: str) -> None:
        with _lock:
            c = self._read(self.collections_path, {})
            c.pop(name, None)
            self._write(self.collections_path, c)

    def delete(self, sample_id: str) -> None:
        data = self.load()
        keep = []
        for s in data["samples"]:
            if s["id"] == sample_id:
                try:
                    os.remove(os.path.join(self.root, s["file"]))
                except FileNotFoundError:
                    pass
            else:
                keep.append(s)
        data["samples"] = keep
        self.save(data)


class Taste:
    """Learns from ratings which search terms and which sound features you like."""

    def __init__(self, data: dict, ratings: dict):
        by_id = {s["id"]: s for s in data["samples"]}
        self.term_weight: dict[str, float] = defaultdict(lambda: 1.0)
        liked, disliked, everything = [], [], []
        for s in data["samples"]:
            everything.append(self._vec(s))
        for sid, v in ratings.items():
            s = by_id.get(sid)
            if not s:
                continue
            if v > 0:
                self.term_weight[s["term"]] += 0.8
                liked.append(self._vec(s))
            else:
                self.term_weight[s["term"]] = max(0.2, self.term_weight[s["term"]] - 0.4)
                disliked.append(self._vec(s))
        self.n_ratings = len(liked) + len(disliked)
        self.center = np.mean(everything, axis=0) if everything else None
        self.like = np.mean(liked, axis=0) if liked else None
        self.dislike = np.mean(disliked, axis=0) if disliked else None

    @staticmethod
    def _vec(s: dict) -> np.ndarray:
        # Loops and phrases are measured differently; both fill eight slots,
        # and the kind itself is a dimension, so they never pass for each other.
        f = s["features"]
        phrase = s.get("kind") == "phrase"
        names = analyze.PHRASE_FEATURES if phrase else analyze.FEATURES
        v = [f.get(k, 0.0) for k in names] + [1.0 if phrase else 0.0]
        v.append(np.log2(max(s.get("bpm") or 100, 40) / 100.0))
        v.append(np.log2(max(s.get("centroid", 1800), 50) / 1800.0))
        return np.array(v, dtype=float)

    def bonus(self, sample: dict) -> float:
        """-10..+10 points: how close a new sample is to what you liked."""
        if self.n_ratings < 3 or self.center is None:
            return 0.0
        v = self._vec(sample) - self.center

        def sim(ref):
            if ref is None:
                return 0.0
            r = ref - self.center
            d = np.linalg.norm(v) * np.linalg.norm(r)
            return float(v @ r / d) if d > 1e-9 else 0.0

        return round(10.0 * np.clip(sim(self.like) - 0.6 * sim(self.dislike), -1, 1), 1)
