# Overnight log

A running note of what changed, so the morning summary is accurate rather than
reconstructed. Newest at the bottom.

---

## On the "make my melody sound cool" plugin idea

The reference is [Cableguys ShaperBox][shaperbox]: eleven rhythmic modules —
volume, pan, width, filter, drive, crush, time, pitch, reverb, noise, liquid —
each driven by a drawable LFO locked to bars and beats. You put a plain part
through it and it comes out sounding produced, because the movement is doing
the work rather than the notes.

**Verdict: mostly a separate plugin, but one part of it belongs here.**

A ShaperBox equivalent processes *any* audio — a vocal, a drum loop, a bounced
stem. That makes it an audio-effect plugin, a different plugin type with a
different bus layout, and squeezing it into a synthesiser would mean it could
only ever process what the synthesiser itself plays. That is the wrong shape
for the tool, so it should be its own project.

But there is a real gap in NOG Suite that the idea exposes, and it is worth
closing regardless:

- **Effect parameters cannot be modulated at all.** Every other part of the
  synth can be, and the effects rack — six slots of filterable, crushable,
  delayable processing — is the part that would benefit most.
- **Every LFO is per-voice.** A per-voice LFO cannot sensibly drive a global
  effect, because there are up to thirty-two of them disagreeing. Rhythmic
  effect movement needs a modulator that belongs to the instrument rather than
  to a note.
- **LFO shapes are a fixed list of seven.** ShaperBox's stepped patterns are
  most of what makes a part sound sequenced rather than merely wobbling.

So the plan for that idea, inside this plugin: global tempo-locked modulators
with step patterns, and the effects rack wired up as modulation destinations.
That gives rhythmic gating, filter patterns, pan movement and stutter on
anything played through NOG Suite — which is the part of the idea that a
synthesiser can honestly deliver.

[shaperbox]: https://www.cableguys.com/shaperbox

---

## Changes

### Output limiter on the master bus

A synthesiser can produce a signal far louder than the patch implies without
anyone having asked for it: a filter pushed into self-oscillation, a resonant
sweep landing on a fundamental, sixteen voices of a patch written for four.
Startling at best, and genuinely dangerous on headphones.

Look-ahead peak limiting at −1 dBFS rather than a clipper. The delay line holds
the signal back by the attack time, so the gain is already down when the peak
arrives — it catches the transient rather than letting the front of it through,
and adds no harmonics doing so. Below the ceiling it is unity gain and does
nothing, which is the normal case.

On by default, because the situation it exists for is exactly the one where
nobody thought to switch it on first. The 1.5 ms of look-ahead is reported to
the host as latency; switching it off removes both.

Three tests: that a signal ten times over the ceiling still comes out at the
ceiling, that a quiet signal passes at exactly its own level, and that a
zero-ramp full-scale burst does not slip past.

## On the pitch-shifter idea

Same answer as ShaperBox, for the same reason: shifting the pitch of an audio
file is something you do to a file, not to a note a synthesiser is playing, so
it wants to be an audio-effect plugin rather than part of an instrument.

Worth knowing though: NOG Suite already pitches a loaded sample across the whole
keyboard, with Catmull-Rom interpolation and per-voice mip selection rather than
the nearest-neighbour resampling a naive sampler does. For one-shots and chops
that is often all that is wanted, and it is already better than dragging a clip.

What it does *not* do is hold the length while changing the pitch, or hold the
formants while changing both. Those need a phase vocoder or PSOLA, which is a
real piece of DSP and belongs in the separate plugin along with the rhythmic
effects — they would share most of their machinery.

### Motion: global step patterns, and an effects rack that can be modulated

Two things were missing, and they were the same thing seen from either end.
Effect parameters could not be modulated at all — the one part of the synth
where movement matters most. And every LFO was per-voice, so even if they could
have been, thirty-two voices would each have had their own opinion about the
mix of a single delay.

Motion is a tempo-locked eight-step pattern that belongs to the instrument
rather than to a note. Two of them, each with rate, smoothing, swing and depth.
A step sequencer rather than a waveform, because that is what makes a part
sound sequenced instead of merely wobbling: a gate on an effect mix, a filter
moving in sixteenths, a pan that hops.

The clock comes from the host playhead rather than being counted locally, so a
pattern stays locked to the bar however the transport is scrubbed or looped.
With the transport stopped it free runs, so patches still audition properly.

Wired through:
- Twenty-four new modulation destinations, four per effect slot.
- A global modulation frame the rack resolves its parameters through.
- Motion published into the matrix alongside the macros, so voices see it too.
- Master gain now read through the modulation rather than straight off the
  parameter, which is what makes a trance gate possible at all.
- A MOTION tab per pattern in the modulators panel: eight vertical sliders and
  three knobs.

Ten patches use it, from a straight trance gate to two patterns running against
each other at different rates.

Tests: that the step reported is a function of the host position and survives a
backwards jump, that a pattern is audible on an effect, and that it gates the
master level.

### Per-slot modulation curves

Every routing was linear, so an envelope could only push a destination in a
straight line. The envelopes already had per-stage curve control; the matrix
did not, which meant the same envelope bent one way driving the amplitude and
another way driving a filter.

The shaping function moved out of Envelope into DSP/Curve.h and is now used by
both, so a routing bends exactly the way an envelope stage does — a routing
shaped by a different function from the envelope driving it would be a surprise
every time anyone compared the two.

Applied to the source before the bipolar re-centring, so an envelope that rises
slowly still rises slowly whichever way the routing points; doing it afterwards
would bend the negative half the opposite way.

Zero by default. A test pins the ends as fixed points of the curve whatever it
is set to, and checks the middle bends in the documented direction — the first
version of that test had the sign backwards, which the code caught.

### A second filter, with serial and parallel routing

The largest remaining gap against Serum and Vital. One filter can shape a
sound; two can split it. A low-pass followed by a high-pass leaves a band,
while the same two summed leave everything except a band — those are opposite
results from the same pair, and the routing switch is what chooses between them.

Off by default, with its own parameter IDs rather than being folded into an
indexed pair, because filter one already shipped under its names and renaming
it would orphan every saved patch. Its four continuous controls are modulation
destinations, appended after the effect block so nothing already saved shifts.

Its panel sits beside the first on the OSC page, in yellow so a glance tells
the two apart. Verified against a screenshot.

A test renders the same patch both ways and asserts parallel passes far more
than serial, which is the audible consequence of the difference.
