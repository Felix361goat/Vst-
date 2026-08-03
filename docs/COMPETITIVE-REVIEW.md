# What Serum 2, Vital and Flex have that NOG Suite does not

A survey of the three synths worth measuring against, and a judgement on which
gaps are worth closing. Written after the wavetable engine and drag-and-drop
modulation landed.

The point is not to match Serum 2 feature for feature — it has had a decade and
a team. The point is to find the gaps that a user would actually notice.

---

## The three benchmarks

- **Serum 2** — the reference. Five oscillator modes (wavetable, sample,
  multisample, granular, spectral), two filters with ~40 types, 10 LFOs, 4
  envelopes, a large effects rack, and roughly 450 factory presets.
- **Vital** — closest in spirit. Comparable wavetable engine, deeper unison
  controls, and a modulation *remap* system that reshapes any source.
- **Flex** — the opposite philosophy: almost no synthesis controls, eight
  labelled macros per preset, and a large categorised preset browser. It is
  popular precisely because it is fast.

---

## Gaps, ranked by what a user would notice first

### 1. No factory presets — the biggest gap by far

Every one of the three ships with a browsable, categorised preset library.
NOG Suite ships with nothing but an init patch, so it opens silent-ish and gives
no impression of what it can do.

This is also the cheapest gap to close: the engine already exists, and a preset
is a list of parameter values. Twenty patches covering bass, lead, pad, pluck,
keys and FX would change the first impression completely — and would exercise
every part of the engine, which is worth something on its own.

**Verdict: do this first.** Highest value per unit of work of anything on this
list.

### 2. Nothing shows the waveform

Serum's rotating 3D wavetable view and Vital's animated oscillator display are
signature. NOG Suite draws its envelopes and LFOs but never shows the actual
wave, the spectrum, or the output.

A wavetable frame display plus a small oscilloscope would do most of the work.
The data is already there — `Wavetable::getSample` can be called straight from
the paint routine.

**Verdict: do this second.** It is what makes a synth feel alive while editing.

### 3. Only factory wavetables — no importing your own

Serum and Vital both load a `.wav` and slice it into frames. NOG Suite has
twelve built-in tables and no way to add to them.

The engine is already built around this: `Wavetable::build` takes harmonic
spectra, and `Wavetable::analyse` already converts a waveform into them. What is
missing is the file loading, the slicing, and the real-time-safe swap of a table
while voices are reading it — that last part is the actual work.

**Verdict: high value, moderate difficulty.**

### 4. One filter, eight types

Serum 2 has two filters and around forty types including comb, flanger, phaser
and formant models. NOG Suite has one filter with eight types from a single
state-variable core.

A second filter with serial/parallel routing, plus a comb filter, would cover
most of the practical difference. The exotic Serum types matter less than having
two filters to route between.

**Verdict: worth doing, in that order.**

### 5. Missing effects

The rack has distortion, chorus, phaser, delay, reverb, EQ and compressor. Set
against the others it is missing:

- **Bit crusher / decimator** — sample-rate and bit-depth reduction. Cheap to
  write, distinctive, and absent from the current rack.
- **Flanger** — currently only reachable as a very short delay.
- **Hyper / Dimension** — Serum's unison-style widener, heavily used on leads.
- **Convolution reverb** — Serum 2 added one; a large job.

Slots also cannot be reordered; the chain is fixed 1 to 6.

**Verdict: bit crusher and flanger are quick wins. Reordering is a UI job.**

### 6. Modulation is linear only

Vital's remap system and Serum 2's editable modulation curves both let a source
be reshaped per routing — a linear envelope driving a destination exponentially,
for instance. NOG Suite applies every routing linearly.

A per-slot curve control, reusing the same shaping function the envelopes
already use, would cover most of it. The matrix slot parameters would need one
more entry, which is a backwards-compatible addition.

**Verdict: moderate value, low difficulty.**

### 7. Fixed LFO shapes, and fewer modulators

Serum 2 has 10 LFOs and 4 envelopes with fully drawable shapes; Vital is
similar. NOG Suite has 4 LFOs and 3 envelopes from a fixed list of 7 shapes.

Raising the counts is nearly free — the constants are in one header, though each
new LFO needs its own modulation destination entry. Drawable shapes are a real
feature: a breakpoint editor, and storing the shape in the patch.

**Verdict: raise the counts cheaply; defer drawable shapes.**

### 8. Unison has no table spread

Vital offsets each unison voice's position within the wavetable, so a stack does
not just detune but also varies in timbre. It is one control and it sounds
considerably richer than detune alone.

**Verdict: cheap, and punches above its weight.**

### 9. No sample, granular or spectral oscillator

Serum 2 has all three; Vital has sample and spectral warping. This is the
largest single gap in raw capability and by far the most work — each is a
synthesis engine in its own right.

**Verdict: defer. Real wavetables first.**

### 10. Smaller things worth noting

- **No MPE.** Vital supports it; increasingly expected.
- **Macros cannot be renamed**, and there are 4 rather than Flex's 8.
- **Preset browser has no categories, tags or author field.**
- **No output limiter.** A safety limiter would stop a self-oscillating filter
  from producing a nasty surprise.
- **Separate pitch bend up/down ranges.**
- **Noise is generated, not sampled** — Serum plays noise samples, which is why
  its noise layer has character.

---

## Recommended order

1. **Factory presets** — biggest perceived improvement, least work.
2. **Wavetable and oscilloscope displays** — makes editing feel alive.
3. **Bit crusher and flanger** — fills the two obvious holes in the rack.
4. **Unison table spread** — one control, large sonic return.
5. **User wavetable import** — the real capability gap.
6. **Second filter** with routing.
7. **Per-slot modulation curves**, then more LFOs and envelopes.
8. **Output limiter**, macro renaming, preset categories.

Sample, granular and spectral oscillators are deliberately last. They are what
separates a good wavetable synth from Serum 2, but everything above makes more
difference per hour spent.
