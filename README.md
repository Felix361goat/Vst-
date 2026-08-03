# NOG Suite

A wavetable and sample synthesiser plugin for FL Studio and other DAWs, built
with JUCE. Serum-shaped: two oscillators that play either a morphing wavetable
or a sample, with unison, a sub and a noise layer, a multi-mode filter, three
envelopes, four LFOs, a 16-slot modulation matrix, four macros, an arpeggiator
and a six-slot effects rack.

It builds on Windows, macOS and Linux, and passes `pluginval` at maximum
strictness on all three.

**Sound sources.** Twelve band-limited wavetables of twenty-four frames each,
built from harmonic spectra with a mip pyramid — measured aliasing sits between
−90 and −105 dB. Alongside them, forty-two generated samples: modelled acoustic
instruments (plucked strings, an additive piano with stretched partials, struck
bars, bowed and blown tones that loop seamlessly), eight-bit sound chips, and
character material a wavetable cannot be, such as transients and noise beds.
None of them are recordings, so there is nothing to download and nothing to
license. Any audio file can be loaded into either oscillator as well.

**Presets.** Just under three hundred factory patches across twenty-one
categories. Every one is unit-tested for level, health and silence on release.

See [`docs/SERUM-FEATURE-MAP.md`](docs/SERUM-FEATURE-MAP.md) for what is built
against what Serum does, and
[`docs/COMPETITIVE-REVIEW.md`](docs/COMPETITIVE-REVIEW.md) for the comparison
against Serum 2, Vital and Flex.

---

## Getting a build

The repository builds on Windows, macOS and Linux. CI builds all three on every
push, so the easiest way to get a plugin is to download one:

1. Open the **Actions** tab on GitHub.
2. Pick the most recent **Build** run.
3. Download the **NOG-Suite-Windows** or **NOG-Suite-macOS** artefact.
4. Unzip it.

## Installing in FL Studio

**Windows** — copy `NOG Suite.vst3` into:

```
C:\Program Files\Common Files\VST3\
```

**macOS** — copy `NOG Suite.vst3` into:

```
/Library/Audio/Plug-Ins/VST3/
```

Then in FL Studio: **Options → Manage plugins → Find more plugins**. Make sure
the folder above is in the search list, then click **Start scan**. NOG Suite will
appear under *Generators*, since it registers as an instrument.

If it does not show up, tick **Verify plugin signatures** off and rescan — FL
Studio caches failed scans, and a rescan with the cache cleared usually finds it.

## Building it yourself

You need CMake 3.22+ and a C++20 compiler. JUCE is fetched automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The results land in:

```
build/NogSuite_artefacts/Release/VST3/NOG Suite.vst3
build/NogSuite_artefacts/Release/Standalone/NOG Suite
```

To have the build install the plugin for you, configure with
`-DNOG_COPY_AFTER_BUILD=ON`.

On Linux you will need the usual JUCE dependencies first:

```bash
sudo apt-get install -y libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
  libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libglu1-mesa-dev mesa-common-dev
```

### Tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

The standalone build is the fastest way to try changes — it opens as a normal
window with its own audio and MIDI settings, no DAW needed.

## Things worth knowing

**The arpeggiator** lives on the GLOBAL page. While it is on, held notes go into
a pattern locked to the host tempo rather than straight to a voice. Gate sets
how long each step rings, swing pushes every second step late. Every patch in
the Arp and Sequence categories has it switched on.

**Samples.** Each oscillator has a mode switch: Wavetable or Sample. In sample
mode the WT Pos knob becomes the playback start offset, the wave selector is
replaced by a LOAD SAMPLE button, and the button's menu holds the forty-two
built-ins plus loop mode and root note. The display in the corner of the panel
shows the file's envelope with a marker at the start offset.

**Via.** Each modulation slot has a second source that scales it rather than
adding to it, which is what lets one modulator control how much another one
does. An LFO on pitch via the mod wheel is vibrato you can play into rather than
a wobble that is always on. Leave it at None for a plain routing.

**Unison spread** fans a stack across the wavetable's frames as well as
detuning it, so the voices differ in timbre and not only in pitch.

**The background** is swappable: the BG button in the top bar loads any image.

## Layout

```
Source/
  PluginProcessor.*        the host-facing plugin
  PluginEditor.*           the window
  Params/                  parameter IDs, layout, cached access
  Modulation/              modulation vocabulary and matrix
  Engine/                  voice, voice allocation, MIDI, arpeggiator
  DSP/                     oscillator, filter, envelope, LFO, noise,
                           wavetables and the generated sample bank
  FX/                      the effects rack
  State/                   preset save and load, the factory bank
  UI/                      look and feel, widgets, panels
Tests/                     engine unit tests
docs/                      architecture and the Serum feature map
```

Start with [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) before adding a module.
The short version: parameter IDs are permanent once shipped, modulation is summed
in normalised parameter space, and the audio thread never allocates.

## Licence

Built on [JUCE](https://juce.com), which is dual-licensed GPLv3 / commercial.
Under the GPLv3 option this plugin's source must be published if it is
distributed. A JUCE licence removes that requirement — worth sorting out before
releasing binaries.
