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

### Twelve more modelled instruments, and the patches for them

Reeds and pipes (harmonium, accordion, pipe organ), horns (trumpet,
saxophone), a shakuhachi that is mostly breath, four more plucked and struck
sources (banjo, koto, hammered dulcimer, celesta), and two hand drums.

The drums needed a new model. A membrane is not a bar or a string: its modes
follow the zeros of a Bessel function, at 1.59, 2.14, 2.30 and so on rather
than at whole multiples. That is why an undamped drum has no clear pitch — and
why a tabla, which is built to suppress most of those modes, does.

Appended to the end of the bank so no existing patch's sample index moved.
Fifty-four generated sources now, every pitched one checked against the note it
declares.

Twenty-five patches, several of which are the first real use of the second
filter and the modulation curves:
- A muted trumpet is a band-pass — a harmon mute takes the bottom out as much
  as the top, which is why one cuts through without being loud.
- A close-miked djembe is the *parallel* case: a low-pass keeping the body and
  a high-pass keeping the slap, summed, leaves the mid scooped out.
- The trumpet and saxophone open their filter through a curve rather than
  linearly, so blowing harder reads as effort rather than as a volume change.

### Noise with character

Filtered white noise is the correct answer to "what does pink noise sound like"
and the wrong answer to "what should a noise layer add to a patch". A real
texture has structure over time, and structure is what makes a noise layer
sound like breath, or tape, or rain, rather than like a hiss with an envelope
on it. Serum plays noise samples for exactly this reason.

Six textured colours join the three generated ones: tape, vinyl and radio reuse
the existing beds, and wind, rain and breath are new. Rain is a steady hiss with
two and a half thousand individual resonant drops scattered over it — the drops
are what stop it being static. Wind is a resonant band whose centre wanders on a
slow random walk, because wind is not loud noise, it is noise that keeps
changing its mind about which frequencies it contains. Breath is periodic: in
for a second and a half, out for longer, then a pause.

Each voice starts its texture at a random position, so several voices playing
the same colour do not run in lockstep and sound like one loud voice.

A test renders every textured colour and requires each to be both audible and
different from white — a colour that quietly fell back to noise would sound
plausible and be wrong.

### Twenty-two more in the sparkle family

The patches the leads-as-chords sound comes from, extended. They share one
shape because that shape is the sound: a wide detuned saw for body, a quiet
bright partial above it for the glint, heavy key tracking so the filter opens
as the line climbs, and a long delay and reverb behind it.

What varies is what supplies the glint — a glockenspiel, a celesta, a piano
hammer, a choir, a koto — how far the filter tracks, and whether the patch is
voiced to be played as a line or held as a chord. Several are deliberately
quieter and slower to open than a lead would be, because a patch voiced for a
single note becomes a wall when six of them sound at once.

Several use the night's new pieces where they earn it: a curve on the filter
envelope so a chord arrives rather than starts, a high-pass under the top line
so it does not muddy the middle, the widener instead of a nine-voice stack so
a wide chord stays in tune, and a Motion pattern on a delay send so the repeats
come and go while the pluck stays even.

### Eighteen more sample sources, and two arpeggiator bugs they exposed

Mallets and tuned metal (vibraphone with its motor tremolo baked in, xylophone,
tubular bell, crotale, gamelan gong, singing bowl, steel tongue drum, wood
block), three more plucked strings (mandolin as a real double course, oud,
guzheng), voices and air (choir "ee", whistle, bottle blow), and four individual
sounds: a water drop whose pitch *rises*, ice, a reversed swell, a sub drop.

Thirty-four patches, most of them arpeggiated, because that is what struck
material is for: a bar has an attack and a decay and nothing in between, so a
held chord does almost nothing with it while a pattern does everything.

Writing them turned up two real arpeggiator bugs that no existing patch happened
to reach:

- **The first note arrived one whole step late.** A step fired at its end rather
  than its start. At a sixteenth that is 125 ms and nobody noticed; the first
  patch to use a quarter-note division sat silent for half a beat after the key
  went down. Steps now fire at their start, and the swung length is held for the
  duration of the step rather than recomputed after the index has moved on.

- **Chord mode never released its notes.** It started every note in the pattern
  but tracked only one as sounding, so the clear-down had nothing to release and
  a chord pattern held voices open for ever. The arp now tracks every note it
  has started, and gates a chord exactly as it gates a single note.
