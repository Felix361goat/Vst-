# What a Serum-class synth needs, and where NOG Suite stands

This is the research behind the foundation: an inventory of Serum's control
surface, module by module, with an honest status for each item.

Status key:

- **Done** — implemented and working.
- **Surface** — the parameter, the UI control and the modulation wiring exist
  and save/load correctly, but the DSP behind it is a placeholder.
- **Todo** — not built yet.

The distinction matters. Anything marked *Surface* costs nothing to finish
later, because the expensive parts — the parameter IDs, the state format, the
modulation routing, the layout — are already fixed. Anything marked *Todo* still
needs new parameters, which is the change that breaks saved patches if it is
done carelessly.

---

## 1. Oscillators (Serum: OSC A / OSC B)

Serum's oscillators are wavetable players: each holds up to 256 single-cycle
frames, and the WT POS control sweeps through them.

| Control | Status | Notes |
|---|---|---|
| Enable | Done | |
| Level | Done | Modulatable |
| Pan | Done | Modulatable, equal-power |
| Wavetable select | Surface | A six-shape analytic bank stands in for loaded tables |
| WT Position | Surface | Morphs across the bank; becomes a frame index with real tables |
| Warp mode | Done | All 8 modes implemented as phase distortion |
| Warp amount | Done | Modulatable |
| Unison voices (1–16) | Done | |
| Unison detune | Done | Modulatable |
| Unison blend | Done | Centre-versus-edges balance |
| Unison stereo width | Done | |
| Phase | Done | Modulatable |
| Phase randomisation | Done | |
| Octave / Semi / Fine | Done | |
| Route to filter | Done | Per-source filter routing |
| Pitch modulation | Done | Virtual destination, ±48 semitones |

**The gap:** real wavetable loading. That means a `.wav` importer that slices a
file into frames, frame interpolation, and per-frame band-limited mipmaps so high
notes do not alias. `Oscillator::sampleForWave` is the single function that has
to change; everything above it stays.

## 2. Sub oscillator

| Control | Status |
|---|---|
| Enable, Level, Pan | Done |
| Waveform (sine/tri/saw/square) | Done |
| Octave | Done |
| Route to filter | Done |

## 3. Noise

| Control | Status | Notes |
|---|---|---|
| Enable, Level, Pan | Done | |
| Colour (white/pink/brown) | Done | Serum plays noise *samples* instead |
| Sample playback, pitch, phase | Todo | Needs a sample loader |
| Route to filter | Done | |

## 4. Filter

| Control | Status | Notes |
|---|---|---|
| Enable | Done | |
| Type | Done | 8 modes from one TPT state-variable core |
| Cutoff | Done | Modulatable, stable under audio-rate sweeps |
| Resonance | Done | Modulatable |
| Drive | Done | Modulatable |
| Mix | Done | Modulatable |
| Key tracking | Done | |
| Per-source routing buttons | Done | |
| Serum's exotic types (comb, flanger, phaser, French LP) | Todo | Additional filter cores |

## 5. Envelopes (Serum: ENV 1–3)

| Control | Status |
|---|---|
| Attack, Hold, Decay, Sustain, Release | Done |
| Per-stage curve controls | Done |
| ENV 1 hard-wired to amplitude | Done |
| Visual editor with draggable handles | Todo (display is read-only) |

## 6. LFOs

Serum has 8 LFOs with fully drawable shapes. This has 4 with a fixed shape list —
the count and the shape editor are both cheap to extend later.

| Control | Status |
|---|---|
| Shape (7 built-in) | Done |
| Rate in Hz | Done, modulatable |
| Tempo sync with divisions | Done |
| Trigger / Envelope / Free-run modes | Done |
| Phase offset | Done |
| Rise (fade-in) | Done |
| Smoothing | Done |
| Bipolar / unipolar | Done |
| Drawable custom shapes | Todo |
| 8 LFOs instead of 4 | Todo (change one constant plus destinations) |

## 7. Modulation matrix

This is the part that is genuinely hard to retrofit, so it was built first.

| Feature | Status | Notes |
|---|---|---|
| 16 slots | Done | Serum has 32 |
| Source, destination, amount per slot | Done | All host-automatable |
| Bipolar switch | Done | |
| Per-slot enable | Done | |
| Per-voice evaluation | Done | Envelopes and LFOs differ per note |
| Modulation shown on the knob | Done | Orange ring around the value arc |
| Drag-and-drop assignment | Todo | Combo boxes for now |
| Per-slot curve / aux source | Todo | |
| 18 sources, 27 destinations | Done | See `Source/Modulation/ModDefs.h` |

Sources: Env 1–3, LFO 1–4, Velocity, Key Track, Mod Wheel, Pitch Bend,
Aftertouch, Random, Macro 1–4.

## 8. Effects rack

Serum has 10 effects in a reorderable rack. This has 6 slots, each of which can
be any of 7 effect types — which covers more combinations, at the cost of not
being able to use two of the same effect in different places without spending two
slots.

| Effect | Status |
|---|---|
| Distortion | Done |
| Chorus | Done |
| Phaser | Done |
| Delay | Done |
| Reverb | Done |
| EQ | Done |
| Compressor | Done |
| Hyper / Dimension | Todo |
| Flanger | Todo |
| Drag-to-reorder slots | Todo (order is fixed 1→6) |

Each slot exposes three generic controls plus a mix, relabelled per effect type.

## 9. Global

| Control | Status | Notes |
|---|---|---|
| Master gain | Done | Modulatable, smoothed |
| Polyphony (1–32) | Done | |
| Mono / Legato / Poly | Done | |
| Glide time and mode | Done | |
| Pitch bend range | Done | |
| Velocity sensitivity | Done | |
| Oversampling | Surface | Parameter and UI exist, no resampling yet |
| Macros 1–4 | Done | |

## 10. Presets

| Feature | Status |
|---|---|
| Save / load to disk | Done |
| Browse, next / previous | Done |
| Init patch | Done |
| Name stored in session state | Done |
| Categories, tags, author metadata | Todo |
| Factory preset bank | Todo |

## 11. Plugin-level requirements

These are the things that decide whether FL Studio loads it at all, rather than
how it sounds.

| Requirement | Status |
|---|---|
| VST3, 64-bit | Done |
| Declared as an instrument (`Instrument\|Synth`) | Done |
| MIDI input accepted, no audio input bus | Done |
| Unique plugin and manufacturer codes | Done |
| Full state save / recall with a version field | Done |
| Editor size remembered | Done |
| Sample-accurate MIDI timing | Done |
| Sustain pedal, pitch bend, mod wheel, aftertouch | Done |
| Any sample rate and block size | Done (tested 22.05–192 kHz, 1–2048 samples) |
| No allocation on the audio thread | Done |
| Passes pluginval at strictness 10 | Done |
| AU build for macOS | Done |
| Standalone build for testing | Done |
| Code signing / installer | Todo |

---

## Suggested order for the next sessions

1. **Real wavetables.** The single biggest step towards sounding like Serum, and
   the architecture is already shaped around it.
2. **Oversampling.** The parameter is already there; wiring
   `juce::dsp::Oversampling` around the voice loop removes the aliasing that a
   hard-driven filter and distortion produce.
3. **Drag-and-drop modulation.** Assigning a source by dragging it onto a knob is
   most of what makes Serum feel fast to program.
4. **Wavetable and spectrum displays.** The 3D table view is Serum's signature.
5. **Factory presets.** Nothing proves the engine works like twenty patches that
   use all of it.
