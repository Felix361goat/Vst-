"""The AI ear: an audio-language model (CLAP) that judges how a sample sounds.

The signal measures in analyze.py can tell whether something is a riff. They
cannot tell a strange, hypnotic riff from an ordinary folk tune, and they
cannot hear crackle. CLAP can: it scores audio against text descriptions.

* weird: how much more it sounds like the descriptions in WEIRD than like the
  ones in ORDINARY;
* grain: how much it sounds like an old, crackly, muffled recording;
* an embedding per sample, which drives "find similar", taste learning and
  searching the library by describing a sound in words.

Optional: without torch and transformers installed, SampleHunter still runs
on the signal measures alone.
"""

from __future__ import annotations

import os
import threading

import numpy as np

MODEL = os.environ.get("HUNTER_CLAP_MODEL", "laion/clap-htsat-unfused")
SR = 48000

WEIRD = [
    "a hypnotic repeating riff on an unusual non-western instrument",
    "a nasal twangy plucked string melody",
    "a haunting vocal melody with ornaments and pitch slides",
    "a buzzing droning reed instrument melody",
    "a shimmering metallic gong and bell melody",
    "a trance-like ritual chant with drums",
    "an unusual catchy melodic hook",
]
ORDINARY = [
    "ordinary folk song with acoustic guitar",
    "a brass band march",
    "a dance band orchestra from the 1920s",
    "a church choir singing a hymn",
    "a classical orchestra",
    "a piano ballad",
    "easy listening music",
    "a pop song",
    "a country song",
    "a person talking",
    "a barbershop quartet",
    "a crooner singing a love song",
]
GRAINY = [
    "old crackling scratchy gramophone record with hiss",
    "a muffled low quality noisy recording",
    "a distorted worn out cassette tape",
]
CLEAN = [
    "a clean clear high quality recording",
    "a crisp modern studio recording",
]

# Raw similarity differences are mapped to 0..100 with a logistic; centres and
# widths come from a calibration run over old records, field recordings, and
# clean and artificially crackled test signals.
WEIRD_CENTER, WEIRD_WIDTH = 0.0, 0.05
GRAIN_CENTER, GRAIN_WIDTH = 0.06, 0.045
# The model hears crackle as "exotic": the same riff scores far weirder with
# surface noise added. This much of the grain measure is taken back out.
GRAIN_BIAS = 0.25

_lock = threading.Lock()
_state: dict = {}


def available() -> bool:
    try:
        import torch  # noqa: F401
        import transformers  # noqa: F401
        return True
    except Exception:
        return False


def _load():
    with _lock:
        if "model" not in _state:
            import torch
            from transformers import ClapModel, ClapProcessor
            torch.set_num_threads(max(1, (os.cpu_count() or 2) - 1))
            model = ClapModel.from_pretrained(MODEL)
            model.eval()
            _state["model"] = model
            _state["proc"] = ClapProcessor.from_pretrained(MODEL)
            prompts = WEIRD + ORDINARY + GRAINY + CLEAN
            _state["prompts"] = _text(prompts)
        return _state["model"], _state["proc"]


def _text(texts: list[str]) -> np.ndarray:
    import torch
    model, proc = _state["model"], _state["proc"]
    with torch.no_grad():
        t = proc(text=texts, return_tensors="pt", padding=True)
        e = model.get_text_features(**t)
        e = e if isinstance(e, torch.Tensor) else e.pooler_output
    e = e.numpy()
    return e / np.linalg.norm(e, axis=1, keepdims=True)


def text_embedding(query: str) -> np.ndarray:
    _load()
    return _text([query])[0]


def listen(clips: list[np.ndarray]) -> list[dict]:
    """clips: mono float arrays at 48 kHz. Returns weird, grain (0..100) and
    an embedding for each."""
    import torch
    model, proc = _load()
    out = []
    for i in range(0, len(clips), 16):
        batch = [c.astype(np.float32) for c in clips[i:i + 16]]
        with torch.no_grad():
            a = proc(audio=batch, sampling_rate=SR, return_tensors="pt")
            e = model.get_audio_features(**a)
            e = e if isinstance(e, torch.Tensor) else e.pooler_output
        e = e.numpy()
        e /= np.linalg.norm(e, axis=1, keepdims=True)
        sims = e @ _state["prompts"].T * 100.0   # CLAP's logit scale, roughly

        nw, no, ng = len(WEIRD), len(ORDINARY), len(GRAINY)
        pw, po = sims[:, :nw] / 100.0, sims[:, nw:nw + no] / 100.0
        pg, pc = sims[:, nw + no:nw + no + ng] / 100.0, sims[:, nw + no + ng:] / 100.0
        top = lambda m: np.sort(m, axis=1)[:, -2:].mean(axis=1)   # noqa: E731
        g_raw = pg.max(axis=1) - pc.max(axis=1)
        w_raw = top(pw) - top(po) - GRAIN_BIAS * np.maximum(g_raw + 0.1, 0)
        for emb, w, g in zip(e, w_raw, g_raw):
            weird = 1 / (1 + np.exp(-(w - WEIRD_CENTER) / WEIRD_WIDTH))
            grain = 1 / (1 + np.exp(-(g - GRAIN_CENTER) / GRAIN_WIDTH))
            out.append({"weird": round(float(100 * weird), 1),
                        "grain": round(float(100 * grain), 1),
                        "emb": [round(float(x), 3) for x in emb]})
    return out
