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
