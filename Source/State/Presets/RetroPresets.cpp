#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        // The sound-chip and nostalgia block, appended to the bank after the
        // acoustic instruments. Indices must stay in step with SampleBank.cpp.
        namespace smp
        {
            constexpr int nesTriangle  = 31;
            constexpr int nesPulse     = 32;
            constexpr int gameBoyWave  = 33;
            constexpr int sidPulse     = 34;
            constexpr int amigaSaw     = 35;
            constexpr int pcSpeaker    = 36;
            constexpr int fmConsole    = 37;
            constexpr int dialUp       = 38;
            constexpr int tapeStop     = 39;
            constexpr int telephone    = 40;
            constexpr int crtHum       = 41;

            constexpr int chipCoin     = 10;
            constexpr int vinylCrackle = 17;
        }
    }

    /**
        Patches built on the sound-chip and nostalgia samples.

        A square wave is a square wave, so what makes these sound like the
        hardware they came from is everything that hardware could not do: four
        bits of output resolution, a clock too slow to avoid aliasing, no
        filter worth the name. Those limits are modelled in the samples; the
        patches here treat them as instruments rather than as sound effects.
    */
    std::vector<Preset> retroPresets()
    {
        std::vector<Preset> list;

        // -- eight-bit ------------------------------------------------------

        // The NES had one triangle channel, no volume control on it, and it
        // played every bass line on the console. Mono for the same reason.
        list.push_back (Build ("NES Bass", "Chiptune")
            .voices (1, 1)
            .sampleOsc (0, smp::nesTriangle, 0.9f, true)
            .oscOff (1)
            .filter (flt::LP24, 2200.0f, 0.05f, 0.0f, 1.0f, 0.25f)
            .env (0, 1.0f, 400.0f, 0.9f, 60.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("NES Sub Bass", "Chiptune")
            .voices (1, 1)
            .glide (30.0f, 2)
            .sampleOsc (0, smp::nesTriangle, 0.85f, true)
            .oscOff (1)
            .sub (0, 0.4f)
            .filter (flt::LP24, 900.0f, 0.08f, 0.15f, 1.0f, 0.2f)
            .env (0, 1.0f, 500.0f, 0.9f, 80.0f)
            .fx (FX::Eq, 1.0f, 0.6f, 0.4f, 0.3f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("NES Lead", "Chiptune")
            .voices (1, 1)
            .sampleOsc (0, smp::nesPulse, 0.8f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 300.0f, 0.85f, 60.0f)
            // Vibrato that arrives late is how a chiptune tracker faked
            // expression: the note is dead straight until it is held.
            .lfoFree (0, 0, 6.5f)
            .lfoShape (0, 240.0f, 0.0f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.012f, true)
            .master (-10.0f)
            // Vibrato the player can reach for: the LFO is already
            // running, and the wheel decides how much of it is heard.
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.022f, true)
            .done());

        // Two chip channels a fifth apart, which is the only chord an NES
        // could afford once the bass and the drums had theirs.
        list.push_back (Build ("Chip Duet Lead", "Chiptune")
            .voices (4)
            .sampleOsc (0, smp::nesPulse, 0.55f, true)
            .osc (1, wt::PulseWidth, 0.15f, 0.35f)
            .tune (1, 0, 7)
            .filter (flt::LP24, 7000.0f, 0.04f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 320.0f, 0.85f, 70.0f)
            .fx (FX::Delay, 0.22f, 0.18f, 0.30f, 0.8f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Chip Arp", "Chiptune")
            .voices (8)
            .arp (0, 8, 2, 0.4f, 0.0f)
            .sampleOsc (0, smp::nesPulse, 0.8f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.06f, 0.0f, 1.0f, 0.4f)
            .env (0, 0.5f, 160.0f, 0.0f, 40.0f)
            .fx (FX::Delay, 0.26f, 0.16f, 0.34f, 0.9f)
            .master (-11.0f)
            .done());

        // The trick every chiptune used to fake a chord: cycle through its
        // notes faster than the ear can separate them.
        list.push_back (Build ("Chip Chord Arp", "Chiptune")
            .voices (8)
            .arp (0, 9, 1, 0.9f, 0.0f)
            .sampleOsc (0, smp::nesPulse, 0.8f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 0.5f, 400.0f, 0.9f, 60.0f)
            .fx (FX::Delay, 0.2f, 0.2f, 0.3f, 0.75f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Game Boy Keys", "Chiptune")
            .voices (8)
            .sampleOsc (0, smp::gameBoyWave, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.06f, 0.05f, 1.0f, 0.45f)
            .env (0, 2.0f, 700.0f, 0.6f, 180.0f)
            .env (1, 1.0f, 240.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.25f)
            .fx (FX::Delay, 0.20f, 0.22f, 0.28f, 0.8f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Game Boy Pluck", "Chiptune")
            .voices (8)
            .sampleOsc (0, smp::gameBoyWave, 0.9f, true)
            .oscOff (1)
            .filter (flt::LP24, 2400.0f, 0.20f, 0.1f, 1.0f, 0.6f)
            .env (0, 0.5f, 260.0f, 0.0f, 120.0f)
            .env (1, 0.5f, 120.0f, 0.0f, 80.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.22f, 0.6f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("SID Lead", "Chiptune")
            .voices (1, 1)
            .glide (25.0f, 2)
            .sampleOsc (0, smp::sidPulse, 0.85f, true)
            .oscOff (1)
            // The SID's filter was famously uneven from chip to chip, and
            // resonant enough to whistle. That is half of its character.
            .filter (flt::LP24, 1800.0f, 0.45f, 0.15f, 1.0f, 0.6f)
            .env (0, 1.0f, 500.0f, 0.7f, 120.0f)
            .env (1, 1.0f, 200.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.40f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.32f, 0.85f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("SID Bass", "Chiptune")
            .voices (1, 1)
            .sampleOsc (0, smp::sidPulse, 0.85f, true)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 700.0f, 0.30f, 0.25f, 1.0f, 0.2f)
            .env (0, 1.0f, 400.0f, 0.8f, 90.0f)
            .fx (FX::Distortion, 0.25f, 0.28f, 0.45f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        // -- sixteen-bit ----------------------------------------------------

        list.push_back (Build ("Amiga Tracker Saw", "Chiptune")
            .voices (6)
            .sampleOsc (0, smp::amigaSaw, 0.8f, true)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.10f, 0.1f, 1.0f, 0.6f)
            .env (0, 1.0f, 600.0f, 0.75f, 160.0f)
            .env (1, 1.0f, 220.0f, 0.0f, 140.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.30f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.22f, 0.6f, 0.3f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Tracker Stab", "Chiptune")
            .voices (10)
            .sampleOsc (0, smp::amigaSaw, 0.7f, true)
            .oscOff (1)
            .filter (flt::LP24, 1600.0f, 0.28f, 0.12f, 1.0f, 0.65f)
            .env (0, 1.0f, 400.0f, 0.0f, 220.0f)
            .env (1, 0.5f, 150.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Delay, 0.22f, 0.24f, 0.30f, 0.8f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("FM Console Bass", "Chiptune")
            .voices (1, 1)
            .sampleOsc (0, smp::fmConsole, 0.85f, true)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 800.0f, 0.20f, 0.25f, 1.0f, 0.25f)
            .env (0, 1.0f, 450.0f, 0.8f, 100.0f)
            .fx (FX::Distortion, 0.3f, 0.3f, 0.45f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("FM Console Lead", "Chiptune")
            .voices (6)
            .sampleOsc (0, smp::fmConsole, 0.8f, true)
            .oscOff (1)
            .filter (flt::LP24, 4500.0f, 0.10f, 0.1f, 1.0f, 0.6f)
            .env (0, 2.0f, 700.0f, 0.7f, 180.0f)
            .env (1, 1.0f, 200.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.30f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("FM Console Bell", "Chiptune")
            .voices (10)
            .sampleOsc (0, smp::fmConsole, 0.6f, true)
            .osc (1, wt::FmBell, 0.38f, 0.3f)
            .tune (0, 1)
            .tune (1, 1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.6f)
            .env (0, 1.0f, 1400.0f, 0.15f, 500.0f)
            .fx (FX::Delay, 0.26f, 0.24f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("PC Speaker Beep", "Chiptune")
            .voices (1, 1)
            .sampleOsc (0, smp::pcSpeaker, 0.7f, true)
            .oscOff (1)
            // A one-bit output through a paper cone had no low end at all, so
            // taking it out is what makes this read as a motherboard.
            .filter (flt::HP12, 700.0f, 0.10f, 0.0f, 1.0f, 0.3f)
            .env (0, 0.5f, 200.0f, 0.9f, 30.0f)
            .master (-13.0f)
            .done());

        // -- nostalgia ------------------------------------------------------

        list.push_back (Build ("Dial Up", "FX")
            .voices (2)
            .sampleOsc (0, smp::dialUp, 0.8f)
            .oscOff (1)
            .filter (flt::BP12, 1600.0f, 0.15f, 0.1f, 1.0f, 0.3f)
            .env (0, 1.0f, 3000.0f, 0.9f, 200.0f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.5f, 0.4f)
            .fx (FX::Reverb, 0.16f, 0.5f, 0.35f, 1.0f)
            .master (-11.0f)
            .done());

        // Pitched down and pushed through a resonant filter, the handshake
        // stops being a modem and becomes a texture.
        list.push_back (Build ("Modem Drone", "Ambient")
            .voices (4)
            .sampleOsc (0, smp::dialUp, 0.6f, true)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (0, -2)
            .tune (1, -1)
            .filter (flt::LP24, 1200.0f, 0.25f, 0.15f, 1.0f, 0.25f)
            .env (0, 800.0f, 3000.0f, 0.85f, 2200.0f)
            .lfoFree (0, 0, 0.09f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.22f, true)
            .fx (FX::Reverb, 0.55f, 0.92f, 0.22f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Tape Stop", "FX")
            .voices (4)
            .sampleOsc (0, smp::tapeStop, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 3000.0f, 0.10f, 0.15f, 1.0f, 0.3f)
            .env (0, 1.0f, 2000.0f, 0.5f, 300.0f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.28f, 0.7f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Telephone Bell", "FX")
            .voices (6)
            .sampleOsc (0, smp::telephone, 0.85f)
            .oscOff (1)
            .filter (flt::BP12, 1200.0f, 0.20f, 0.1f, 1.0f, 0.5f)
            .env (0, 1.0f, 2200.0f, 0.6f, 260.0f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.3f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Old Phone Bell Keys", "Bell")
            .voices (8)
            .sampleOsc (0, smp::telephone, 0.7f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 4000.0f, 0.08f, 0.05f, 1.0f, 0.55f)
            .env (0, 0.5f, 900.0f, 0.0f, 300.0f)
            .fx (FX::Delay, 0.24f, 0.22f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("CRT Room Tone", "Ambient")
            .voices (2)
            .sampleOsc (0, smp::crtHum, 0.7f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.2f)
            .env (0, 900.0f, 3000.0f, 0.9f, 2000.0f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.45f, 0.3f)
            .fx (FX::Reverb, 0.4f, 0.85f, 0.26f, 1.0f)
            .master (-14.0f)
            .done());

        // -- lo-fi hybrids --------------------------------------------------
        //
        // The chip source on one oscillator and something modern on the other:
        // the quantisation reads as texture rather than as a gimmick when
        // there is a full-bandwidth sound sitting next to it.

        list.push_back (Build ("Lofi Chip Keys", "Keys")
            .voices (10)
            .sampleOsc (0, smp::gameBoyWave, 0.45f, true)
            .osc (1, wt::BasicShapes, 0.30f, 0.35f)
            .tune (1, 0)
            .noise (1, 0.03f, false)
            .filter (flt::LP24, 1800.0f, 0.06f, 0.05f, 1.0f, 0.4f)
            .env (0, 4.0f, 1600.0f, 0.35f, 420.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.38f, 0.3f)
            .fx (FX::Reverb, 0.34f, 0.74f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Chip Sparkle Lead", "Chiptune")
            .voices (8)
            .sampleOsc (0, smp::nesPulse, 0.30f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.45f)
            .tune (0, 1)
            .unison (1, 7, 0.16f, 0.55f, 0.9f)
            .filter (flt::LP24, 3400.0f, 0.09f, 0.06f, 1.0f, 0.8f)
            .env (0, 3.0f, 1000.0f, 0.70f, 420.0f)
            .env (1, 1.0f, 240.0f, 0.0f, 240.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.26f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.6f)
            .fx (FX::Delay, 0.26f, 0.28f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Arcade Bell Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::chipCoin, 0.35f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.4f)
            .tune (0, -1)
            .filter (flt::LP24, 4000.0f, 0.06f, 0.0f, 1.0f, 0.6f)
            .env (0, 2.0f, 1400.0f, 0.4f, 600.0f)
            .fx (FX::Delay, 0.24f, 0.26f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Dusty Chip Pad", "Pad")
            .voices (10)
            .sampleOsc (0, smp::sidPulse, 0.35f, true)
            .osc (1, wt::Formant, 0.32f, 0.35f)
            .tune (1, 0)
            .unison (1, 5, 0.12f, 0.55f, 0.9f)
            .noise (1, 0.04f, false)
            .filter (flt::LP24, 1600.0f, 0.06f, 0.0f, 1.0f, 0.35f)
            .env (0, 700.0f, 2800.0f, 0.8f, 1800.0f)
            .lfoFree (0, 0, 0.11f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.16f, true)
            .fx (FX::Chorus, 0.3f, 0.08f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.52f, 0.90f, 0.22f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Nostalgia Keys", "Keys")
            .voices (10)
            .sampleOsc (0, smp::vinylCrackle, 0.12f, true)
            .osc (1, wt::FmBell, 0.30f, 0.5f)
            .tune (1, 0)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 4.0f, 2200.0f, 0.25f, 600.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.4f, 0.3f)
            .fx (FX::Delay, 0.18f, 0.28f, 0.26f, 0.75f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        return list;
    }
}
