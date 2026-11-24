"""Local BPM + key detection. numpy/scipy only; uses librosa automatically if installed.

Key: Krumhansl-Kessler profiles correlated with an averaged chroma vector.
BPM: spectral-flux onset envelope -> autocorrelation, with half/double alternatives
(producers often count trap at 140 where an algorithm hears 70).
"""
from __future__ import annotations

from pathlib import Path

import numpy as np
from scipy.signal import resample_poly

from .knowledge import KEY_NAMES

SR = 22050
MAJOR = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
MINOR = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])
# Camelot wheel: index = pitch class
CAMELOT_MAJOR = ["8B", "3B", "10B", "5B", "12B", "7B", "2B", "9B", "4B", "11B", "6B", "1B"]
CAMELOT_MINOR = ["5A", "12A", "7A", "2A", "9A", "4A", "11A", "6A", "1A", "8A", "3A", "10A"]

try:  # optional, more accurate
    import librosa  # type: ignore
    HAVE_LIBROSA = True
except Exception:
    HAVE_LIBROSA = False


def load(path: Path, max_seconds: float = 90.0) -> np.ndarray:
    if HAVE_LIBROSA:
        y, _ = librosa.load(str(path), sr=SR, mono=True)
    else:
        import soundfile as sf
        data, sr = sf.read(str(path), always_2d=True, dtype="float32")
        y = data.mean(axis=1)
        if sr != SR:
            from math import gcd
            g = gcd(int(sr), SR)
            y = resample_poly(y, SR // g, int(sr) // g).astype(np.float32)
    # analyse the middle part (skip intro / producer tag)
    n = int(max_seconds * SR)
    if len(y) > n:
        start = (len(y) - n) // 2
        y = y[start:start + n]
    return y


def _stft_mag(y: np.ndarray, n_fft: int = 4096, hop: int = 512) -> np.ndarray:
    win = np.hanning(n_fft).astype(np.float32)
    frames = 1 + max(0, (len(y) - n_fft) // hop)
    idx = np.arange(n_fft)[None, :] + hop * np.arange(frames)[:, None]
    return np.abs(np.fft.rfft(y[idx] * win, axis=1)).T  # (bins, frames)


def detect_bpm(y: np.ndarray) -> tuple[float, float]:
    hop = 512
    if HAVE_LIBROSA:
        env = librosa.onset.onset_strength(y=y, sr=SR, hop_length=hop)
    else:
        mag = _stft_mag(y, n_fft=2048, hop=hop)
        logm = np.log1p(100 * mag)
        flux = np.maximum(0, np.diff(logm, axis=1)).sum(axis=0)
        env = flux - np.convolve(flux, np.ones(16) / 16, mode="same")
        env = np.maximum(env, 0)
    env = env - env.mean()
    ac = np.correlate(env, env, mode="full")[len(env) - 1:]
    fps = SR / hop
    ac = ac / (ac[0] + 1e-9)
    x = np.arange(len(ac))
    bpms = np.arange(60.0, 200.0, 0.25)
    lags = fps * 60.0 / bpms

    def at(lag):
        return np.interp(lag, x, ac, right=0.0)

    # a true beat period is also strong at 2x and 4x its lag (bars line up)
    score = at(lags) + 0.5 * at(2 * lags) + 0.25 * at(4 * lags)
    prior = np.exp(-0.5 * (np.log2(bpms / 125.0) / 1.0) ** 2)
    score = score * prior
    best = int(np.argmax(score))
    # octave check: prefer double tempo if it is nearly as strong (hi-hats / 8ths drive trap)
    dbl = int(np.argmin(np.abs(bpms - 2 * bpms[best])))
    if bpms[best] * 2 <= 180 and abs(bpms[dbl] - 2 * bpms[best]) < 1 and score[dbl] > 0.8 * score[best]:
        best = dbl
    conf = float((score[best] - np.median(score)) / (score.max() + 1e-9))
    return float(bpms[best]), max(0.0, min(1.0, conf))


def chroma(y: np.ndarray) -> np.ndarray:
    if HAVE_LIBROSA:
        yh = librosa.effects.harmonic(y)
        return librosa.feature.chroma_cqt(y=yh, sr=SR).mean(axis=1)
    n_fft = 8192
    mag = _stft_mag(y, n_fft=n_fft, hop=2048)
    freqs = np.fft.rfftfreq(n_fft, 1 / SR)
    sel = (freqs > 55) & (freqs < 2000)
    midi = 69 + 12 * np.log2(freqs[sel] / 440.0)
    pc = np.mod(np.round(midi), 12).astype(int)
    energy = np.log1p(mag[sel] ** 2).mean(axis=1)
    out = np.zeros(12)
    np.add.at(out, pc, energy)
    return out


def detect_key(y: np.ndarray) -> dict:
    c = chroma(y)
    c = (c - c.mean()) / (c.std() + 1e-9)
    results = []
    for tonic in range(12):
        for mode, prof in (("major", MAJOR), ("minor", MINOR)):
            p = np.roll(prof, tonic)
            p = (p - p.mean()) / p.std()
            results.append((float(np.corrcoef(c, p)[0, 1]), tonic, mode))
    results.sort(reverse=True)
    r, tonic, mode = results[0]
    second = results[1]
    name = f"{KEY_NAMES[tonic]} {'Major' if mode == 'major' else 'Minor'}"
    if mode == "minor":
        rel = f"{KEY_NAMES[(tonic + 3) % 12]} Major"
        camelot = CAMELOT_MINOR[tonic]
    else:
        rel = f"{KEY_NAMES[(tonic + 9) % 12]} Minor"
        camelot = CAMELOT_MAJOR[tonic]
    alt = f"{KEY_NAMES[second[1]]} {'Major' if second[2] == 'major' else 'Minor'}"
    return {"key": name, "relative": rel, "camelot": camelot, "confidence": round(max(0.0, r), 2),
            "margin": round(r - second[0], 3), "alternative": alt}


def analyze(path: Path) -> dict:
    y = load(path)
    if len(y) < SR * 3:
        raise ValueError("Audio ist zu kurz (mindestens 3 Sekunden)")
    bpm, bpm_conf = detect_bpm(y)
    key = detect_key(y)
    alts = sorted({round(bpm / 2, 1), round(bpm * 2, 1)} - {round(bpm, 1)})
    return {"bpm": round(bpm), "bpm_exact": round(bpm, 1), "bpm_alternatives": [a for a in alts if 50 <= a <= 220],
            "bpm_confidence": round(bpm_conf, 2), **key, "engine": "librosa" if HAVE_LIBROSA else "numpy",
            "duration_analyzed": round(len(y) / SR, 1)}
