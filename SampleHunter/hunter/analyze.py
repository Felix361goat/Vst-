"""Finding the riff in a recording.

What makes the "Get Ur Freak On" loop work, put in terms a computer can measure:

* it repeats: the same short figure, bar after bar, so it loops by itself;
* it moves: a melody of a handful of notes, not a drone and not a solo;
* it is clear: one pitched voice you can hear, not a wash of harmony;
* it is syncopated: the notes sit between the beats as much as on them;
* it is plucked: sharp attacks with a pitched tail;
* it is nasal and bright, in the mid-range where a tumbi, banjo or kora sits.

Every candidate window of one or two bars is scored on those, and the
"Freak score" (0–100) is their weighted combination. Windows are cut on the
beat grid, so a sample loops at the BPM it is labelled with.
"""

from __future__ import annotations

import subprocess
from dataclasses import dataclass

import numpy as np

SR = 22050
HOP = 512
MAX_SECONDS = 480

FEATURES = ["repeat", "motion", "clarity", "syncopation", "punch",
            "brightness", "harmonic", "tonal"]
WEIGHTS = np.array([0.25, 0.12, 0.12, 0.14, 0.10, 0.10, 0.08, 0.09])

_MAJOR = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
_MINOR = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])
_NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


@dataclass
class Window:
    start: float          # seconds
    duration: float       # seconds
    beats: int            # length in beats (4/4)
    bpm: float
    key: str
    score: float          # 0..100
    features: dict
    centroid: float
    kind: str = "loop"    # "loop": cut on the beat; "phrase": a free one-shot line


def decode(path: str, sr: int = SR, channels: int = 1, start: float = 0.0,
           duration: float | None = MAX_SECONDS) -> np.ndarray:
    cmd = ["ffmpeg", "-v", "error", "-nostdin"]
    if start > 0:
        cmd += ["-ss", "%.6f" % start]
    cmd += ["-i", path]
    if duration:
        cmd += ["-t", "%.6f" % duration]
    cmd += ["-ac", str(channels), "-ar", str(sr), "-f", "f32le", "-"]
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    data = np.frombuffer(raw, dtype=np.float32)
    if channels > 1:
        data = data.reshape(-1, channels)
    return data.copy()


def _key(chroma_mean: np.ndarray) -> str:
    best, name = -2.0, "?"
    c = chroma_mean - chroma_mean.mean()
    for i in range(12):
        for prof, mode in ((_MAJOR, "maj"), (_MINOR, "min")):
            p = np.roll(prof, i)
            r = np.corrcoef(c, p - p.mean())[0, 1]
            if r > best:
                best, name = r, "%s %s" % (_NOTES[i], mode)
    return name


def _cos(a: np.ndarray, b: np.ndarray) -> float:
    a = a - a.mean()
    b = b - b.mean()
    d = np.linalg.norm(a) * np.linalg.norm(b)
    return float(a @ b / d) if d > 1e-9 else 0.0


def _clip(x):
    return float(np.clip(x, 0.0, 1.0))


def _bars_for(period: float, tempo_hint: float) -> float:
    """How many 4/4 bars one repetition period most plausibly spans."""
    best, best_err = 1.0, 1e9
    for bars in (0.5, 1.0, 2.0, 4.0):
        bpm = 240.0 * bars / period
        err = min(abs(np.log2(bpm / t)) for t in (tempo_hint, tempo_hint * 2, tempo_hint / 2))
        err += 0.25 * abs(np.log2(bpm / 105.0))   # pull towards hip-hop tempi
        if not 68 <= bpm <= 175:
            err += 2.0
        if err < best_err:
            best, best_err = bars, err
    return best


def excerpt_start(path: str, rng=None) -> float:
    """Where to start analysing. Long recordings (concerts, tapes) get a random
    stretch rather than always their opening, which is often tuning or talk."""
    import random
    try:
        out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                              "-of", "default=nw=1:nk=1", path], capture_output=True, text=True).stdout
        dur = float(out.strip())
    except (ValueError, OSError):
        return 0.0
    if dur <= MAX_SECONDS + 30:
        return 0.0
    return (rng or random).uniform(min(60.0, dur - MAX_SECONDS), dur - MAX_SECONDS)


def find_windows(path: str, max_windows: int = 3, min_score: float = 55.0,
                 offset: float = 0.0) -> list[Window]:
    import librosa  # imported here so the rest of the tool starts fast
    from scipy.ndimage import uniform_filter1d, maximum_filter1d

    y = decode(path, start=offset)
    if len(y) < SR * 15:
        return []
    y = y / (np.max(np.abs(y)) + 1e-9)

    S = np.abs(librosa.stft(y, n_fft=2048, hop_length=HOP))
    H, P = librosa.decompose.hpss(S, margin=1.0)
    oenv = librosa.onset.onset_strength(S=librosa.amplitude_to_db(S, ref=np.max), sr=SR)
    tempo_hint = float(np.atleast_1d(librosa.feature.tempo(onset_envelope=oenv, sr=SR, hop_length=HOP))[0])

    chroma = librosa.feature.chroma_stft(S=H ** 2, sr=SR, hop_length=HOP)
    rms = librosa.feature.rms(S=S, frame_length=2048)[0]
    cent = librosa.feature.spectral_centroid(S=S, sr=SR)[0]
    flat = librosa.feature.spectral_flatness(S=S)[0]
    he = (H ** 2).sum(axis=0)
    pe = (P ** 2).sum(axis=0)
    T = min(chroma.shape[1], len(oenv), len(rms))
    fps = SR / HOP

    cn = chroma[:, :T] / (chroma[:, :T].sum(axis=0, keepdims=True) + 1e-9)
    Z = uniform_filter1d(cn, 3, axis=1)
    Z = Z - Z.mean(axis=0, keepdims=True)
    Z /= np.linalg.norm(Z, axis=0, keepdims=True) + 1e-9
    e = oenv[:T] / (np.percentile(oenv[:T], 99) + 1e-9)
    e_tol = maximum_filter1d(e, 3)            # forgive a frame of timing slop
    track_rms = np.median(rms[:T]) + 1e-9

    peaks = librosa.onset.onset_detect(onset_envelope=oenv, sr=SR, hop_length=HOP)
    peaks = peaks[peaks < T]
    peaks = peaks[e[peaks] > 0.25]
    if len(peaks) == 0:
        return []
    # Start each loop just before the attack, not on its peak.
    strong = np.unique(librosa.onset.onset_backtrack(peaks, rms[:T]))
    lags = np.arange(int(1.2 * fps), int(7.0 * fps))
    starts = strong[strong < T - 2 * lags[0]]
    if len(starts) == 0:
        return []

    # Chroma self-similarity at every lag, averaged over a window one lag long.
    rep = np.full((len(lags), len(starts)), -1.0)
    for i, l in enumerate(lags):
        sim = np.einsum("kt,kt->t", Z[:, :T - l], Z[:, l:])
        cs = np.concatenate([[0.0], np.cumsum(sim)])
        ok = starts + l <= T - l
        st = starts[ok]
        rep[i, ok] = (cs[st + l] - cs[st]) / l
    contrast = rep - np.median(np.where(rep > -1, rep, np.nan), axis=0, keepdims=True)

    # Keep the best period per start, then the most promising starts.
    best_lag = np.argmax(rep, axis=0)
    cols = np.arange(len(starts))
    order = np.argsort(-rep[best_lag, cols])[:300]

    cands: list[Window] = []
    for j in order:
        li = best_lag[j]
        l = int(lags[li])
        t0 = int(starts[j])
        if t0 + 2 * l > T:
            continue
        r_abs = _clip(rep[li, j])
        r_con = _clip(contrast[li, j] / 0.35)
        ow, on = e[t0:t0 + l], e_tol[t0 + l:t0 + 2 * l]
        r_on = _clip(_cos(ow, on))
        if not np.isfinite(r_abs + r_con + r_on):
            continue
        repeat = 0.35 * r_abs + 0.35 * r_con + 0.30 * r_on

        period = l / fps
        bars = _bars_for(period, tempo_hint)
        # Loop whole periods until it is at least a bar, and two when short.
        reps = 2 if bars < 1 else 1
        if period * reps < 3.0 and bars * reps <= 1:
            reps *= 2
        nbeats = int(round(4 * bars * reps))
        bpm = 240.0 * bars / period
        dur = period * reps
        seg = slice(t0, t0 + int(round(dur * fps)))
        if rms[seg].mean() < 0.4 * track_rms:
            continue

        cw = cn[:, seg]
        ew = e[seg]
        pk = ew > max(0.25, np.percentile(ew, 70))
        k = len(set(np.argmax(cw[:, pk], axis=0).tolist())) if pk.any() else 0
        motion = {0: 0.0, 1: 0.2, 2: 0.6, 3: 0.95, 4: 1.0, 5: 1.0, 6: 0.9, 7: 0.6}.get(k, 0.35)

        clarity = _clip((np.mean(cw.max(axis=0)) - 0.15) / 0.30)

        steps = max(8, int(round(16 * bars)))
        pos = t0 + np.round(np.arange(steps) * l / steps).astype(int)
        grid_e = e_tol[np.minimum(pos, T - 1)]
        idx = np.arange(steps) % 4
        ratio = grid_e[idx != 0].mean() / (grid_e[idx == 0].mean() + 1e-9)
        synco = _clip(ratio / 0.75) * (1.0 if ratio < 1.6 else 0.7)

        punch = _clip(((np.percentile(ew, 92) - np.median(ew)) / (ew.mean() + 1e-9)) / 1.5)

        c_med = float(np.median(cent[seg]))
        bright = float(np.exp(-(np.log2(max(c_med, 50) / 1800.0) ** 2) / (2 * 0.8 ** 2)))

        hr = he[seg].sum() / (he[seg].sum() + pe[seg].sum() + 1e-9)
        harm = _clip(1 - abs(hr - 0.72) / 0.35)

        w_e = rms[seg] ** 2
        tonal = _clip(1 - float((flat[seg] * w_e).sum() / (w_e.sum() + 1e-12)) / 0.25)

        feats = np.array([repeat, motion, clarity, synco, punch, bright, harm, tonal])
        score = float(WEIGHTS @ feats)
        rw = rms[seg]
        stability = _clip((np.percentile(rw, 15) / (np.percentile(rw, 90) + 1e-9)) / 0.35)
        score *= 0.6 + 0.4 * stability
        score *= 0.5 + 0.5 * tonal          # crackle-only and speech fall out here
        score *= 0.3 + 0.7 * _clip((repeat - 0.35) / 0.45)   # it has to loop

        cands.append(Window(
            start=offset + t0 / fps, duration=dur, beats=nbeats, bpm=bpm,
            key=_key(cw.mean(axis=1)), score=round(100 * min(score / 0.85, 1.0), 1),
            features={f: round(float(v), 3) for f, v in zip(FEATURES, feats)},
            centroid=c_med))

    cands.sort(key=lambda w: w.score, reverse=True)
    picked: list[Window] = []
    for w in cands:
        if w.score < min_score:
            break
        if any(abs(w.start - p.start) < max(w.duration, p.duration) + 2.0 for p in picked):
            continue
        picked.append(w)
        if len(picked) >= max_windows:
            break
    return picked


def align(reference: str, other: str, at: float, span: float = 6.0) -> float:
    """Seconds to add to a time in `reference` to find the same spot in `other`.

    Lossy and lossless files of one recording can be offset by encoder
    padding; the cut has to land on the same attack in both."""
    t0 = max(0.0, at - span / 2)
    a = decode(reference, start=t0, duration=span)
    b = decode(other, start=max(0.0, t0 - 0.25), duration=span + 0.5)
    if len(a) < SR or len(b) < len(a):
        return 0.0
    n = len(b) + len(a)
    nfft = 1 << (n - 1).bit_length()
    corr = np.fft.irfft(np.fft.rfft(b, nfft) * np.conj(np.fft.rfft(a, nfft)), nfft)[: len(b) - len(a) + 1]
    lag = int(np.argmax(corr))
    return (lag / SR) - (t0 - max(0.0, t0 - 0.25))


def write_audio(dest: str, audio: np.ndarray, sr: int) -> None:
    """WAV (24-bit) or MP3 (320 kbps, with the LAME gapless header) by extension."""
    if dest.lower().endswith(".mp3"):
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "f32le", "-ar", str(sr), "-ac", "2",
                        "-i", "-", "-c:a", "libmp3lame", "-b:a", "320k", dest],
                       input=np.ascontiguousarray(audio, dtype=np.float32).tobytes(), check=True)
    else:
        import soundfile as sf
        sf.write(dest, audio, sr, subtype="PCM_24")


def to_wav(src: str, dest: str, duration: float | None = None) -> None:
    """Decode a stored sample to a WAV of exactly its loop length (MP3 encoder
    padding is cut off, so a loop stays a perfect loop)."""
    audio = decode(src, sr=44100, channels=2, duration=duration)
    if duration:
        n = int(round(duration * 44100))
        if len(audio) < n:
            audio = np.vstack([audio, np.zeros((n - len(audio), 2), np.float32)])
        audio = audio[:n]
    write_audio(dest, audio, 44100)


def finish(audio: np.ndarray, kind: str = "loop", sr: int = 44100) -> np.ndarray:
    """The same treatment for every cut: DC off, a 4 ms fade in, a fade out
    (4 ms for a loop, 120 ms for a phrase), peak at -1 dBFS."""
    audio = audio - audio.mean(axis=0, keepdims=True)
    fade = int(0.004 * sr)
    audio[:fade] *= np.linspace(0, 1, fade, dtype=np.float32)[:, None]
    tail = fade if kind == "loop" else int(0.12 * sr)
    audio[-tail:] *= np.linspace(1, 0, tail, dtype=np.float32)[:, None] ** (1 if kind == "loop" else 2)
    peak = np.max(np.abs(audio)) + 1e-9
    return (audio * (10 ** (-1 / 20) / peak)).astype(np.float32)


def peaks(audio: np.ndarray, n: int = 160) -> list[float]:
    mono = np.abs(audio).max(axis=1)
    return [round(float(b.max()), 3) for b in np.array_split(mono, n)]


def cut(path: str, w: Window, sr: int = 44100, offset: float = 0.0) -> np.ndarray:
    """Cut the window from the source at full quality, finished."""
    audio = decode(path, sr=sr, channels=2, start=max(0.0, w.start + offset), duration=w.duration)
    length = int(round(w.duration * sr))
    if len(audio) < length:
        audio = np.vstack([audio, np.zeros((length - len(audio), 2), np.float32)])
    return finish(audio[:length], w.kind, sr)


def export(path: str, w: Window, dest: str, sr: int = 44100, offset: float = 0.0) -> list[float]:
    """Cut and write a sample. Returns waveform peaks."""
    audio = cut(path, w, sr, offset)
    write_audio(dest, audio, sr)
    return peaks(audio)


def clicks(audio: np.ndarray, sr: int = 44100) -> float:
    """Clicks and crackle per second: short spikes in the top octaves that
    stand far above their surroundings. Clean recordings sit near 0; worn
    records and damaged tapes show tens per second."""
    from scipy.signal import butter, sosfilt
    from scipy.ndimage import median_filter
    mono = audio.mean(axis=1) if audio.ndim == 2 else audio
    hp = sosfilt(butter(4, 6000, "highpass", fs=sr, output="sos"), mono)
    env = np.abs(hp)
    floor = median_filter(env, size=int(0.01 * sr)) + 1e-6
    spikes = env > 8 * floor
    # Count onsets of spike runs, not every sample in them.
    starts = np.flatnonzero(spikes[1:] & ~spikes[:-1])
    return float(len(starts) / (len(mono) / sr))


def describe(w: Window, vintage: bool) -> list[str]:
    f = w.features
    if w.kind == "phrase":
        tags = ["Phrase", "%.0f s" % w.duration]
        tags.append("hell" if w.centroid > 2200 else "warm" if w.centroid < 1100 else "mittig")
        if f["melisma"] > 0.7:
            tags.append("melismatisch")
        if f["exposed"] > 0.75:
            tags.append("solo/frei")
        if f["range"] > 0.8:
            tags.append("großer Ambitus")
        if vintage:
            tags.append("vintage")
        return tags
    bars = w.beats / 4
    tags = ["%g Takt%s" % (bars, "" if bars == 1 else "e")]
    tags.append("hell" if w.centroid > 2200 else "warm" if w.centroid < 1100 else "mittig")
    if f["punch"] > 0.6 and f["harmonic"] > 0.5:
        tags.append("plucky")
    if f["syncopation"] > 0.8:
        tags.append("synkopiert")
    if f["repeat"] > 0.75:
        tags.append("loopt sauber")
    if f["motion"] >= 0.95:
        tags.append("melodisch")
    if vintage:
        tags.append("vintage")
    return tags


# --- phrases -------------------------------------------------------------------
#
# The other kind of sample: not a loop but one line, sung or played freely,
# like the Carnatic vocal phrases producers have flipped for decades. What
# makes one usable: a single clear pitched voice, exposed (little else going
# on), that moves: ornaments, slides, a range of a few notes, between held
# tones; and that starts and ends cleanly, inside 1.5–8 seconds.

PHRASE_FEATURES = ["voiced", "melisma", "range", "steady", "clarity", "exposed",
                   "tonal", "brightness"]
PHRASE_WEIGHTS = np.array([0.20, 0.20, 0.12, 0.06, 0.12, 0.14, 0.10, 0.06])


def find_phrases(path: str, max_phrases: int = 2, min_score: float = 55.0,
                 offset: float = 0.0) -> list[Window]:
    import librosa
    from scipy.ndimage import uniform_filter1d

    y = decode(path, start=offset)
    if len(y) < SR * 10:
        return []
    y = y / (np.max(np.abs(y)) + 1e-9)
    fps = SR / HOP

    S = np.abs(librosa.stft(y, n_fft=2048, hop_length=HOP))
    H, P = librosa.decompose.hpss(S, margin=1.0)
    chroma = librosa.feature.chroma_stft(S=H ** 2, sr=SR, hop_length=HOP)
    rms = librosa.feature.rms(S=S, frame_length=2048)[0]
    flat = librosa.feature.spectral_flatness(S=S)[0]
    cent = librosa.feature.spectral_centroid(S=S, sr=SR)[0]
    he = (H ** 2).sum(axis=0)
    pe = (P ** 2).sum(axis=0)
    T = min(chroma.shape[1], len(rms))
    cn = chroma[:, :T] / (chroma[:, :T].sum(axis=0, keepdims=True) + 1e-9)

    # Cheap per-frame "one clear voice" measure, to shortlist regions.
    clar = cn.max(axis=0)
    hr = he[:T] / (he[:T] + pe[:T] + 1e-9)
    loud = rms[:T] > 0.35 * np.median(rms[:T])
    lead = uniform_filter1d(clar * hr * (1 - np.minimum(flat[:T] / 0.25, 1)) * loud, int(0.5 * fps))
    gate = lead > max(0.15, 0.6 * np.percentile(lead, 90))

    regions = []
    t = 0
    while t < T:
        if not gate[t]:
            t += 1
            continue
        u = t
        while u < T and gate[u]:
            u += 1
        regions.append((t, u))
        t = u
    # Split long regions at their quietest point, drop short ones.
    out_regions = []
    max_len, min_len = int(8 * fps), int(1.5 * fps)
    stack = list(regions)
    while stack:
        a, b = stack.pop()
        if b - a < min_len:
            continue
        if b - a > max_len:
            lo = a + min_len + int(np.argmin(uniform_filter1d(rms[a + min_len:b - min_len], 5))) if b - a > 2 * min_len else a + max_len
            stack += [(a, lo), (lo, b)]
            continue
        out_regions.append((a, b))
    out_regions.sort(key=lambda r: -lead[r[0]:r[1]].mean())

    cands: list[Window] = []
    for a, b in out_regions[:8]:
        seg = y[a * HOP:b * HOP]
        f0, voiced, _ = librosa.pyin(seg, fmin=70, fmax=1100, sr=SR, frame_length=2048, hop_length=HOP)
        vf = float(np.mean(voiced))
        semis = 12 * np.log2(f0[voiced] / 440.0) if voiced.any() else np.array([])
        if len(semis) < 10:
            continue
        rng = float(np.percentile(semis, 95) - np.percentile(semis, 5))
        d = np.abs(np.diff(semis)) * fps
        d = d[d < 40 * fps / 4]                      # octave jumps are tracking errors
        move = float(np.median(d)) if len(d) else 0.0
        mean_move = float(np.mean(d)) if len(d) else 0.0
        steady_frac = float(np.mean(d < 1.5)) if len(d) else 0.0
        sl = slice(a, b)

        voiced_s = _clip((vf - 0.35) / 0.45)
        melisma = _clip(mean_move / 14.0) * (0.6 + 0.4 * _clip(move / 3.0))
        range_s = _clip(rng / 4.0) if rng < 12 else _clip(1 - (rng - 12) / 12)
        steady = _clip(1 - abs(steady_frac - 0.45) / 0.45)
        clarity = _clip((np.mean(clar[sl]) - 0.2) / 0.35)
        exposed = _clip((np.mean(hr[sl]) - 0.55) / 0.35)
        w_e = rms[sl] ** 2
        tonal = _clip(1 - float((flat[sl] * w_e).sum() / (w_e.sum() + 1e-12)) / 0.25)
        c_med = float(np.median(cent[sl]))
        bright = float(np.exp(-(np.log2(max(c_med, 50) / 1500.0) ** 2) / (2 * 0.9 ** 2)))

        feats = np.array([voiced_s, melisma, range_s, steady, clarity, exposed, tonal, bright])
        score = float(PHRASE_WEIGHTS @ feats)
        score *= 0.5 + 0.5 * voiced_s           # there has to be a line to hear
        score *= 0.35 + 0.65 * _clip(0.6 * melisma + 0.4 * range_s)   # and it has to move
        score = round(100 * min(score, 1.0), 1)

        # Start just before the first voiced attack, end after the last.
        vidx = np.flatnonzero(voiced)
        start = (a + max(0, vidx[0] - 3)) / fps
        end = min(T, a + vidx[-1] + int(0.25 * fps)) / fps
        cands.append(Window(start=offset + start, duration=end - start, beats=0, bpm=0.0,
                            key=_key(cn[:, sl].mean(axis=1)), score=score,
                            features={k: round(float(v), 3) for k, v in zip(PHRASE_FEATURES, feats)},
                            centroid=c_med, kind="phrase"))

    cands.sort(key=lambda w: w.score, reverse=True)
    picked = []
    for w in cands:
        if w.score < min_score or len(picked) >= max_phrases:
            break
        if w.duration < 1.2:
            continue
        if any(abs(w.start - p.start) < 3 for p in picked):
            continue
        picked.append(w)
    return picked


def grain_dsp(path: str, w: Window) -> float:
    """Crackle/muffle estimate without the AI ear: old records and worn tapes
    lose their top end, so the frequency below which 95 % of the energy sits
    is low. 0 = clean, 100 = very lo-fi."""
    import librosa
    y = decode(path, sr=44100, start=w.start, duration=min(w.duration, 10.0))
    if len(y) < 2048:
        return 0.0
    roll = librosa.feature.spectral_rolloff(y=y, sr=44100, roll_percent=0.95)[0]
    rms = librosa.feature.rms(y=y)[0][:len(roll)]
    loud = roll[rms > np.median(rms)] if (rms > np.median(rms)).any() else roll
    return round(float(np.clip((9000 - np.median(loud)) / 6000, 0, 1) * 100), 1)
