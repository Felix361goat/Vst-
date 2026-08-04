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
