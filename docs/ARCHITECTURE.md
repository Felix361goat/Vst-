# Architecture

How the code is arranged, and why it is arranged that way. Read this before
adding a module — most of the decisions here exist to make later additions cheap.

## The layers

```
PluginProcessor         host-facing: buses, state, transport
  └── SynthEngine       MIDI, voice allocation, master gain, FX
        ├── Voice ×32   one sounding note
        │     ├── Oscillator ×2 + sub, NoiseGenerator, StateVariableFilter
        │     ├── Envelope ×3, Lfo ×4
        │     └── ModulationFrame       this voice's modulation for the sub-block
        ├── ModMatrix   the routing table, shared by all voices
        └── FXChain     6 slots × 7 effect types

ParameterStore          typed, cached pointers into the APVTS
PresetManager           patches on disk
```

Nothing below `SynthEngine` knows what a host is, and nothing below `Voice`
knows what a parameter ID looks like. That is what lets `Tests/EngineTests.cpp`
run the real synthesis code without a DAW.

## Parameters

Every host-visible parameter is declared once, in
`Source/Params/ParameterLayout.cpp`, and addressed by an ID built in
`Source/Params/ParameterIDs.h`. There are 240 of them.

**Parameter IDs are permanent API.** Once a patch has been saved with an ID,
renaming it orphans that value in every project a user has. Add new parameters
freely — a state file missing a parameter falls back to its default, so adding is
backwards compatible. Renaming or removing is not.

On the audio thread, parameters are read through `ParameterStore`, which resolves
every pointer once in `attach()` and hands out typed members afterwards. A read
is then a single atomic load. Nothing on the audio thread ever hashes a string.

## Modulation

The design constraint: modulation must be per-voice (two notes held together have
different envelope positions), must not disturb the automation the host sees, and
must not cost a string lookup per sample.

The solution has three parts:

1. **`ModMatrix::refresh()`** runs once per block and collapses the 16 slot
   parameters into a compact list of active routings. Empty and zero-amount slots
   drop out, so the usual patch leaves two or three entries.

2. **`ModulationFrame`** is per-voice state: an array of source values indexed by
   `mod::Source`, and an array of offsets indexed by `mod::Dest`. Applying the
   matrix is a walk over the short routing list adding into a flat array.

3. **`ParameterStore::modulated()`** combines a base parameter value with an
   offset. Modulation is summed in **normalised 0..1 parameter space**, then
   converted back to real units. This is what makes one "amount" knob mean the
   same proportion of travel whether it is driving a 20 Hz–20 kHz cutoff or a
   -1..1 pan, and it makes clamping trivial.

Modulation runs at **control rate**: every 32 samples (`Voice::modulationBlock`),
not every sample. Amplitude is interpolated across the sub-block so fast attacks
do not step. This cuts the matrix cost by more than an order of magnitude with no
audible difference.

### Virtual destinations

Most destinations map to a parameter. Pitch does not — a note's frequency comes
from octave, semitone, fine tune and the key played, so there is no single
parameter to nudge. `mod::isVirtual()` marks those, and they scale their offset
by a fixed full-scale amount (±48 semitones) which the voice adds directly.

**To add a modulation destination:** add the enum entry immediately before
`Count` in `ModDefs.h`, add its display name to the matching table (a
`static_assert` fails the build if you forget), map it to a parameter in
`ParameterStore::attach`, and read it where it applies. Do not insert entries in
the middle — the choice index is what gets serialised into saved patches.

## Voices

`SynthEngine` owns a fixed pool of 32 voices, allocated once. The polyphony
parameter chooses how many of them are used, so changing polyphony never
allocates.

Allocation order when a note arrives: a free voice, then the quietest voice that
is already releasing, then the oldest voice. The last two are stolen with a 6 ms
release rather than cut, which is the difference between a soft transition and a
click.

MIDI is handled sample-accurately: `SynthEngine::process` splits the block at
every event and renders the audio between events. At a 2048-sample buffer, not
doing this would put notes up to 45 ms out of place.

## Real-time safety

The audio thread never allocates, locks or blocks:

- The voice pool is fixed and allocated in `prepare`.
- `ModMatrix::routings` reserves its capacity in the constructor.
- **Every effect type is constructed for every slot up front**, so changing a
  slot's type is a pointer swap rather than a `new`. This costs memory and buys
  the guarantee.
- Parameter reads are atomic loads.
- `juce::ScopedNoDenormals` wraps `processBlock`.

## State

`getStateInformation` writes the APVTS tree plus a `stateVersion` property.
`setStateInformation` checks the tag matches, then hands the tree back. The
migration point for future format changes is marked in `PluginProcessor.cpp`.

Editor size lives in the same tree, so a window reopens where the user left it.

Presets are the same XML written to a folder under Documents, which is what lets
a patch move between machines and between the plugin and standalone builds.

## UI

`Source/UI/` holds the look and feel, four reusable widgets, and one panel per
module. Panels own their controls as members and are attached to parameters via
`AudioProcessorValueTreeState` attachments, so no panel holds state that can drift
out of sync with the engine.

Two things worth knowing:

- **`ComboBoxAttachment` does not populate the box.** `ChoiceBox` copies the
  items from the parameter itself, which is also what guarantees the list can
  never disagree with the parameter driving it.
- Knobs advertise their modulation depth through a slider property, which the
  look and feel reads to draw the orange ring. That keeps `NogLookAndFeel`
  independent of the modulation system.

## Testing

`Tests/EngineTests.cpp` builds the engine sources into a console app with a
stand-in `AudioProcessor`. It covers parameter resolution and uniqueness, state
round-tripping, modulation routing and clamping, envelope behaviour including the
control-rate path, oscillator output health across every wave and warp, voice
limits, the sustain pedal, every effect type, and a sweep of sample rates and
block sizes from 22.05 kHz to 192 kHz and 1 to 2048 samples.

CI additionally runs `pluginval` at strictness 10 on all three platforms.
