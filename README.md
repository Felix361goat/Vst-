# NOG Suite

A wavetable synthesiser plugin for FL Studio and other DAWs, built with JUCE.
Serum-shaped: two morphing oscillators with unison, a sub and a noise layer, a
multi-mode filter, three envelopes, four LFOs, a 16-slot modulation matrix, four
macros and a six-slot effects rack.

**Current state — foundation.** It builds, loads, plays, and passes `pluginval`
at maximum strictness. The full control surface, the modulation system, state
saving and the UI are done. The oscillators generate their waves analytically
rather than from loaded wavetables, so it makes sound and is useful for testing,
but it does not sound like Serum yet.

See [`docs/SERUM-FEATURE-MAP.md`](docs/SERUM-FEATURE-MAP.md) for exactly what is
built, what is a placeholder, and what is next.

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

## Layout

```
Source/
  PluginProcessor.*        the host-facing plugin
  PluginEditor.*           the window
  Params/                  parameter IDs, layout, cached access
  Modulation/              modulation vocabulary and matrix
  Engine/                  voice, voice allocation, MIDI
  DSP/                     oscillator, filter, envelope, LFO, noise
  FX/                      the effects rack
  State/                   preset save and load
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
