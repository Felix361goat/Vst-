"""Samples that live in the cloud, not on your disk.

A sample in the library does not have to be a file. It can be a pointer: the
recording it was cut from (a public URL on the Internet Archive or Wikimedia),
where it starts and how long it is. Playing it fetches just those seconds;
the file only lands on disk when you drag or download it, or when you make it
a favourite, which keeps a copy in case the source ever goes away.

Seeking inside a remote file is not always sample-exact (variable-bitrate MP3s
in particular), so every pointer carries an anchor: the loudness envelope of
the sample's first two seconds at 500 Hz. The fetched audio, with a margin on
either side, is slid against it and cut where it matches.
"""

from __future__ import annotations

import base64
import os
import threading

import numpy as np

from . import analyze

SR = 44100
ENV_RATE = 500           # anchor resolution: 2 ms
ANCHOR_SECONDS = 2.0
MARGIN = 1.0             # seconds fetched either side of the cut
CACHE_LIMIT = 400 * 1024 * 1024

_locks: dict[str, threading.Lock] = {}
_locks_guard = threading.Lock()


def _envelope(mono: np.ndarray, sr: int = SR) -> np.ndarray:
    hop = sr // ENV_RATE
    n = len(mono) // hop
    if n == 0:
        return np.zeros(0)
    frames = mono[: n * hop].reshape(n, hop)
    return np.sqrt((frames ** 2).mean(axis=1) + 1e-12)


def make_anchor(audio: np.ndarray, sr: int = SR) -> str:
    """Compact fingerprint of a sample's opening, as base64 of log-scaled bytes."""
    mono = audio.mean(axis=1) if audio.ndim == 2 else audio
    env = _envelope(mono[: int(ANCHOR_SECONDS * sr)], sr)
    db = 20 * np.log10(env / (env.max() + 1e-12) + 1e-6)
    q = np.clip((db + 60) / 60 * 255, 0, 255).astype(np.uint8)
    return base64.b64encode(q.tobytes()).decode()


def _anchor_env(anchor: str) -> np.ndarray:
    q = np.frombuffer(base64.b64decode(anchor), dtype=np.uint8).astype(float)
    return q / 255 * 60 - 60


def _lock(key: str) -> threading.Lock:
    with _locks_guard:
        return _locks.setdefault(key, threading.Lock())


def render(sample: dict, hq: bool = False) -> np.ndarray:
    """Fetch and cut a cloud sample. Returns stereo float audio at 44.1 kHz,
    finished exactly like a sample cut during the hunt."""
    st = sample["stream"]
    url = st.get("hq_url") if hq and st.get("hq_url") else st["url"]
    start = st["start"] + (st.get("hq_offset", 0.0) if url == st.get("hq_url") else 0.0)
    dur = st["duration"]
    t0 = max(0.0, start - MARGIN)
    lead = start - t0
    audio = analyze.decode(url, sr=SR, channels=2, start=t0, duration=dur + lead + MARGIN)
    if len(audio) < int(0.5 * dur * SR):
        raise IOError("Quelle liefert zu wenig Audio")

    shift = 0
    if st.get("anchor"):
        # Loops repeat, so the anchor also fits a beat or a bar away. Seeking
        # is usually exact; only move when a nearby fit is clearly better
        # than none, and never by more than 0.3 s.
        ref = _anchor_env(st["anchor"])
        env = _envelope(audio.mean(axis=1))
        db = 20 * np.log10(env / (env.max() + 1e-12) + 1e-6)
        hop = SR // ENV_RATE
        expect = int(round(lead * SR / hop))
        a = ref - ref.mean()

        def fit(lag):
            seg = db[expect + lag:expect + lag + len(ref)]
            if expect + lag < 0 or len(seg) < len(ref):
                return -np.inf
            b = seg - seg.mean()
            return float(a @ b) / (np.linalg.norm(a) * np.linalg.norm(b) + 1e-9)

        reach = int(0.3 * SR / hop)
        scores = {lag: fit(lag) for lag in range(-reach, reach + 1)}
        best_lag = max(scores, key=scores.get)
        if scores[best_lag] > max(0.6, scores[0] + 0.08):
            shift = best_lag * hop
    i0 = int(round(lead * SR)) + shift
    i0 = max(0, i0)
    n = int(round(dur * SR))
    cut = audio[i0:i0 + n]
    if len(cut) < n:
        cut = np.vstack([cut, np.zeros((n - len(cut), 2), np.float32)])
    return analyze.finish(cut, st.get("kind", "loop"))


class Store:
    """Where cloud samples are kept on disk once fetched.

    _stream/  previews: a cache with a size limit, oldest dropped first
    _kept/    favourites and anything dragged out: never dropped
    """

    def __init__(self, root: str):
        self.stream_dir = os.path.join(root, "_stream")
        self.kept_dir = os.path.join(root, "_kept")
        os.makedirs(self.stream_dir, exist_ok=True)
        os.makedirs(self.kept_dir, exist_ok=True)

    def _paths(self, sample: dict) -> tuple[str, str]:
        name = sample["id"] + ".mp3"
        return os.path.join(self.kept_dir, name), os.path.join(self.stream_dir, name)

    def local(self, sample: dict) -> str | None:
        for p in self._paths(sample):
            if os.path.exists(p):
                return p
        return None

    def preview(self, sample: dict) -> str:
        """A playable file for the sample, fetching it if needed."""
        with _lock(sample["id"]):
            have = self.local(sample)
            if have:
                os.utime(have)
                return have
            _, dest = self._paths(sample)
            analyze.write_audio(dest, render(sample), SR)
        self._trim()
        return dest

    def keep(self, sample: dict) -> str:
        """Keep a full-quality copy for good (favourites, drags, downloads)."""
        with _lock(sample["id"]):
            kept, cached = self._paths(sample)
            if os.path.exists(kept):
                return kept
            sample_hq = bool(sample["stream"].get("hq_url"))
            if os.path.exists(cached) and not sample_hq:
                os.replace(cached, kept)
            else:
                analyze.write_audio(kept, render(sample, hq=True), SR)
                if os.path.exists(cached):
                    os.remove(cached)
            return kept

    def release(self, sample: dict) -> None:
        """No longer a favourite: back into the size-limited cache."""
        kept, cached = self._paths(sample)
        if os.path.exists(kept):
            os.replace(kept, cached)
            self._trim()

    def _trim(self) -> None:
        files = [os.path.join(self.stream_dir, f) for f in os.listdir(self.stream_dir)]
        files = [(os.path.getmtime(f), os.path.getsize(f), f) for f in files if os.path.isfile(f)]
        total = sum(s for _, s, _ in files)
        for _, size, f in sorted(files):
            if total <= CACHE_LIMIT:
                break
            try:
                os.remove(f)
                total -= size
            except OSError:
                pass

    def usage(self) -> dict:
        def size(d):
            return sum(os.path.getsize(os.path.join(d, f)) for f in os.listdir(d))
        return {"cache_mb": round(size(self.stream_dir) / 1e6, 1),
                "kept_mb": round(size(self.kept_dir) / 1e6, 1)}
