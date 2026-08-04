#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        namespace smp
        {
            constexpr int grandPiano   = 5;
            constexpr int kalimba      = 7;
            constexpr int marimbaBar   = 9;
            constexpr int glockenspiel = 25;
            constexpr int choirOo      = 30;
            constexpr int koto         = 49;
            constexpr int dulcimer     = 50;
            constexpr int celesta      = 51;
        }
    }

    /**
        The sparkle family, extended.

        These share one shape, because that shape is the sound: a wide detuned
        saw for body, a quiet bright partial above it for the glint, heavy key
        tracking so the filter opens as the line climbs, and a long delay and
        reverb behind it. What varies is what supplies the glint, how far the
        filter tracks, and whether the patch is voiced to be played as a line or
        held as a chord.

        Several are deliberately quieter and slower to open than a lead would
        be. A patch voiced for a single note becomes a wall when six of them
        sound at once, and holding chords is what these get used for.
    */
    std::vector<Preset> sparklePresets()
    {
        std::vector<Preset> list;

        // -- chords ----------------------------------------------------------

        list.push_back (Build ("Sparkle Chords Bright", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.50f)
            .osc (1, wt::OddEven, 0.55f, 0.24f)
            .tune (1, 1, 0, 4.0f)
            .unison (0, 7, 0.14f, 0.55f, 0.92f, 0.12f)
            .filter (flt::LP24, 3400.0f, 0.09f, 0.06f, 1.0f, 0.75f)
            .env (0, 10.0f, 1300.0f, 0.74f, 620.0f)
            .env (1, 4.0f, 320.0f, 0.22f, 360.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.26f, 0.35f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.74f, 0.58f)
            .fx (FX::Delay, 0.20f, 0.30f, 0.32f, 0.78f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.25f, 1.0f)
            .master (-12.0f)
            .done());

        // The same voicing an octave lower and rolled off: chords that sit
        // under a vocal rather than over one.
        list.push_back (Build ("Sparkle Chords Deep", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::HarmonicSweep, 0.62f, 0.20f)
            .tune (0, -1)
            .tune (1, 0)
            .unison (0, 7, 0.15f, 0.55f, 0.90f, 0.14f)
            .filter (flt::LP24, 1500.0f, 0.10f, 0.06f, 1.0f, 0.6f)
            .env (0, 18.0f, 1600.0f, 0.76f, 760.0f)
            .fx (FX::Eq, 1.0f, 0.50f, 0.58f, 0.45f)
            .fx (FX::Delay, 0.18f, 0.32f, 0.28f, 0.72f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // A real piano attack under the stack. The hammer gives the chord an
        // edge no saw has, and the saw gives it a sustain no piano has.
        list.push_back (Build ("Sparkle Piano Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.50f)
            .osc (1, wt::BasicShapes, wt::saw, 0.30f)
            .tune (1, 0)
            .unison (1, 7, 0.14f, 0.55f, 0.90f, 0.12f)
            .filter (flt::LP24, 2800.0f, 0.08f, 0.05f, 1.0f, 0.65f)
            .env (0, 6.0f, 2400.0f, 0.55f, 700.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.70f, 0.52f)
            .fx (FX::Delay, 0.18f, 0.30f, 0.30f, 0.75f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.25f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Sparkle Vox Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::choirOo, 0.45f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.34f)
            .tune (1, 1)
            .unison (1, 7, 0.15f, 0.55f, 0.92f, 0.16f)
            .filter (flt::LP24, 2600.0f, 0.08f, 0.05f, 1.0f, 0.68f)
            .env (0, 25.0f, 1600.0f, 0.76f, 800.0f)
            .lfoFree (0, 0, 4.4f)
            .lfoShape (0, 1500.0f, 0.25f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.004f, true)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.55f)
            .fx (FX::Delay, 0.18f, 0.32f, 0.28f, 0.72f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.23f, 1.0f)
            .master (-12.0f)
            .done());

        // A struck bar an octave up is the cleanest glint there is: it arrives
        // with the chord and is gone before the chord is, so it reads as an
        // attack rather than as a second instrument.
        list.push_back (Build ("Sparkle Glass Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::glockenspiel, 0.30f)
            .osc (1, wt::BasicShapes, wt::saw, 0.44f)
            .tune (0, 1)
            .unison (1, 7, 0.14f, 0.55f, 0.90f, 0.12f)
            .filter (flt::LP24, 3000.0f, 0.08f, 0.05f, 1.0f, 0.72f)
            .env (0, 8.0f, 1400.0f, 0.72f, 640.0f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.76f, 0.58f)
            .fx (FX::Delay, 0.22f, 0.30f, 0.34f, 0.82f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Sparkle Celesta Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::celesta, 0.34f)
            .osc (1, wt::BasicShapes, 0.42f, 0.42f)
            .tune (0, 1)
            .unison (1, 5, 0.12f, 0.55f, 0.88f, 0.18f)
            .filter (flt::LP24, 2800.0f, 0.07f, 0.04f, 1.0f, 0.66f)
            .env (0, 12.0f, 1600.0f, 0.68f, 720.0f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.72f, 0.55f)
            .fx (FX::Delay, 0.20f, 0.32f, 0.30f, 0.78f)
            .fx (FX::Reverb, 0.42f, 0.84f, 0.23f, 1.0f)
            .master (-12.0f)
            .done());

        // Width from the widener rather than from a nine-voice stack, so the
        // chord stays in tune while still filling the sides.
        list.push_back (Build ("Sparkle Wide Chords", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::HarmonicSweep, 0.76f, 0.24f)
            .tune (1, 1, 0, 4.0f)
            .unison (0, 3, 0.09f, 0.5f, 0.6f, 0.10f)
            .filter (flt::LP24, 3000.0f, 0.09f, 0.05f, 1.0f, 0.72f)
            .env (0, 12.0f, 1400.0f, 0.74f, 680.0f)
            .fx (FX::Dimension, 0.55f, 0.50f, 0.26f, 0.95f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.72f, 0.55f)
            .fx (FX::Delay, 0.18f, 0.32f, 0.30f, 0.75f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // The filter swells in through a curve rather than a straight line, so
        // a held chord arrives rather than simply starting.
        list.push_back (Build ("Sparkle Rise Chords", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::OddEven, 0.60f, 0.22f)
            .tune (1, 1)
            .unison (0, 7, 0.15f, 0.55f, 0.92f, 0.14f)
            .filter (flt::LP24, 900.0f, 0.14f, 0.06f, 1.0f, 0.6f)
            .env (0, 30.0f, 1800.0f, 0.78f, 900.0f)
            .env (1, 700.0f, 2200.0f, 0.85f, 900.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.55f, -0.7f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.72f, 0.55f)
            .fx (FX::Delay, 0.20f, 0.32f, 0.32f, 0.80f)
            .fx (FX::Reverb, 0.42f, 0.86f, 0.23f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Sparkle Warm Chords", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.42f, 0.55f)
            .osc (1, wt::FmBell, 0.28f, 0.18f)
            .tune (1, 1)
            .unison (0, 5, 0.12f, 0.55f, 0.88f, 0.20f)
            // Tape underneath rather than white noise: a texture with structure
            // reads as a recording, and a hiss reads as a fault.
            .noise (3, 0.06f, false)
            .filter (flt::LP24, 1900.0f, 0.07f, 0.04f, 1.0f, 0.58f)
            .env (0, 22.0f, 1700.0f, 0.72f, 820.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.56f, 0.42f)
            .fx (FX::Delay, 0.18f, 0.34f, 0.28f, 0.72f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // A very wide fan: the edges of the stack are a different waveform from
        // the centre, so the chord keeps moving for as long as it is held.
        list.push_back (Build ("Sparkle Fan Chords", "Chords")
            .voices (12)
            .osc (0, wt::HarmonicSweep, 0.45f, 0.58f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.20f)
            .tune (1, 1)
            .unison (0, 9, 0.16f, 0.58f, 0.95f, 0.65f)
            .filter (flt::LP24, 2600.0f, 0.08f, 0.05f, 1.0f, 0.66f)
            .env (0, 20.0f, 1700.0f, 0.74f, 820.0f)
            .lfoFree (0, 0, 0.13f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.16f, true)
            .fx (FX::Eq, 1.0f, 0.46f, 0.70f, 0.55f)
            .fx (FX::Delay, 0.20f, 0.32f, 0.32f, 0.80f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.23f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Sparkle Trance Chords", "Trance")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.62f)
            .osc (1, wt::BasicShapes, wt::saw, 0.34f)
            .tune (1, 0, 0, 12.0f)
            .unison (0, 7, 0.18f, 0.58f, 0.95f, 0.10f)
            .filter (flt::LP24, 2600.0f, 0.16f, 0.10f, 1.0f, 0.70f)
            .env (0, 4.0f, 1000.0f, 0.78f, 460.0f)
            .env (1, 2.0f, 260.0f, 0.20f, 300.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.30f, 0.3f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.72f, 0.58f)
            .fx (FX::Delay, 0.24f, 0.26f, 0.36f, 0.88f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.25f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Sparkle Drill Chords", "Drill")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .sampleOsc (1, smp::marimbaBar, 0.26f)
            .tune (1, 1)
            .unison (0, 5, 0.12f, 0.55f, 0.88f, 0.12f)
            .filter (flt::LP24, 2400.0f, 0.10f, 0.06f, 1.0f, 0.68f)
            .env (0, 3.0f, 900.0f, 0.30f, 480.0f)
            .env (1, 1.0f, 260.0f, 0.0f, 220.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.32f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.70f, 0.55f)
            .fx (FX::Delay, 0.20f, 0.20f, 0.30f, 0.78f)
            .fx (FX::Reverb, 0.32f, 0.76f, 0.26f, 1.0f)
            .master (-11.0f)
            .done());

        // -- leads -----------------------------------------------------------

        list.push_back (Build ("Sparkle Lead Bright", "Lead")
            .voices (8)
            .osc (0, wt::BasicShapes, wt::saw, 0.62f)
            .osc (1, wt::Digital, 0.26f, 0.26f)
            .tune (1, 1)
            .unison (0, 7, 0.16f, 0.55f, 0.90f, 0.12f)
            .filter (flt::LP24, 3600.0f, 0.10f, 0.08f, 1.0f, 0.88f)
            .env (0, 3.0f, 900.0f, 0.76f, 400.0f)
            .env (1, 1.0f, 240.0f, 0.24f, 280.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.28f, 0.4f)
            .routeCurved (Src::Velocity, Dst::FilterCutoff, 0.18f, -0.3f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.78f, 0.62f)
            .fx (FX::Delay, 0.24f, 0.28f, 0.36f, 0.88f)
            .fx (FX::Reverb, 0.32f, 0.76f, 0.25f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Sparkle Lead Soft", "Lead")
            .voices (8)
            .osc (0, wt::BasicShapes, 0.40f, 0.58f)
            .osc (1, wt::FmBell, 0.26f, 0.20f)
            .tune (1, 1)
            .unison (0, 5, 0.12f, 0.55f, 0.86f, 0.16f)
            .filter (flt::LP24, 2200.0f, 0.07f, 0.03f, 1.0f, 0.70f)
            .env (0, 14.0f, 1200.0f, 0.74f, 560.0f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.64f, 0.50f)
            .fx (FX::Delay, 0.22f, 0.32f, 0.32f, 0.82f)
            .fx (FX::Reverb, 0.40f, 0.84f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        // Very high and very thin: the line that sits above everything else in
        // the arrangement without taking any room from it.
        list.push_back (Build ("Sparkle Top Line", "Lead")
            .voices (6)
            .osc (0, wt::BasicShapes, wt::saw, 0.50f)
            .osc (1, wt::OddEven, 0.70f, 0.28f)
            .tune (0, 1)
            .tune (1, 2)
            .unison (0, 5, 0.13f, 0.55f, 0.88f, 0.14f)
            // A high-pass under it is what keeps a top line from muddying the
            // middle: there is nothing down there it needs.
            .filter (flt::LP24, 5000.0f, 0.07f, 0.03f, 1.0f, 0.9f)
            .filter2 (flt::HP24, 700.0f, 0.10f)
            .env (0, 4.0f, 900.0f, 0.72f, 420.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.80f, 0.62f)
            .fx (FX::Delay, 0.26f, 0.26f, 0.38f, 0.92f)
            .fx (FX::Reverb, 0.38f, 0.82f, 0.22f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Sparkle Sub Lead", "Lead")
            .voices (6)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::HarmonicSweep, 0.74f, 0.24f)
            .tune (1, 1, 0, 5.0f)
            .unison (0, 5, 0.13f, 0.55f, 0.86f, 0.12f)
            // A sub under a lead is what makes it usable on its own: the line
            // carries its own bottom instead of needing a bass part.
            .sub (0, 0.32f)
            .filter (flt::LP24, 2800.0f, 0.10f, 0.06f, 1.0f, 0.75f)
            .env (0, 4.0f, 1000.0f, 0.76f, 440.0f)
            .fx (FX::Eq, 1.0f, 0.50f, 0.70f, 0.52f)
            .fx (FX::Delay, 0.22f, 0.28f, 0.32f, 0.82f)
            .fx (FX::Reverb, 0.30f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Sparkle Koto Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::koto, 0.42f)
            .osc (1, wt::BasicShapes, wt::saw, 0.36f)
            .tune (1, 0)
            .unison (1, 5, 0.13f, 0.55f, 0.88f, 0.14f)
            .filter (flt::LP24, 3000.0f, 0.08f, 0.05f, 1.0f, 0.78f)
            .env (0, 2.0f, 1400.0f, 0.55f, 480.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.74f, 0.56f)
            .fx (FX::Delay, 0.24f, 0.26f, 0.34f, 0.86f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Sparkle Dulcimer Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::dulcimer, 0.40f)
            .osc (1, wt::BasicShapes, wt::saw, 0.38f)
            .tune (1, 0)
            .unison (1, 7, 0.15f, 0.55f, 0.90f, 0.12f)
            .filter (flt::LP24, 3400.0f, 0.08f, 0.05f, 1.0f, 0.80f)
            .env (0, 2.0f, 1300.0f, 0.50f, 460.0f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.76f, 0.58f)
            .fx (FX::Delay, 0.24f, 0.28f, 0.34f, 0.88f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Sparkle Kalimba Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::kalimba, 0.40f)
            .osc (1, wt::BasicShapes, 0.44f, 0.34f)
            .tune (1, 0)
            .unison (1, 5, 0.12f, 0.55f, 0.86f, 0.16f)
            .filter (flt::LP24, 3200.0f, 0.07f, 0.04f, 1.0f, 0.76f)
            .env (0, 2.0f, 1200.0f, 0.45f, 420.0f)
            .fx (FX::Delay, 0.24f, 0.24f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.25f, 1.0f)
            .master (-10.0f)
            .done());

        // -- plucks ------------------------------------------------------------

        list.push_back (Build ("Sparkle Pluck Bright", "Pluck")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.60f)
            .osc (1, wt::OddEven, 0.58f, 0.24f)
            .tune (1, 1)
            .unison (0, 5, 0.12f, 0.55f, 0.88f, 0.12f)
            .filter (flt::LP24, 1500.0f, 0.16f, 0.06f, 1.0f, 0.75f)
            .env (0, 1.0f, 640.0f, 0.06f, 360.0f)
            .env (1, 0.5f, 200.0f, 0.0f, 160.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.45f, 0.5f)
            .routeCurved (Src::Velocity, Dst::FilterCutoff, 0.18f, -0.3f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.56f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.25f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Sparkle Pluck Wide", "Pluck")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.58f)
            .osc (1, wt::FmBell, 0.32f, 0.22f)
            .tune (1, 1)
            .unison (0, 3, 0.10f, 0.5f, 0.62f, 0.10f)
            .filter (flt::LP24, 1700.0f, 0.15f, 0.06f, 1.0f, 0.72f)
            .env (0, 1.0f, 700.0f, 0.08f, 400.0f)
            .env (1, 0.5f, 220.0f, 0.0f, 180.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.42f)
            .fx (FX::Dimension, 0.50f, 0.42f, 0.32f, 0.95f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.25f, 1.0f)
            .master (-10.0f)
            .done());

        // A pattern on the delay send rather than on the note, so the repeats
        // come and go while the pluck itself stays even.
        list.push_back (Build ("Sparkle Motion Pluck", "Pluck")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.58f)
            .osc (1, wt::OddEven, 0.56f, 0.22f)
            .tune (1, 1)
            .unison (0, 5, 0.12f, 0.55f, 0.88f, 0.12f)
            .filter (flt::LP24, 1600.0f, 0.15f, 0.06f, 1.0f, 0.72f)
            .env (0, 1.0f, 660.0f, 0.06f, 380.0f)
            .env (1, 0.5f, 200.0f, 0.0f, 170.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.42f)
            .motion (0, 7, { 0.1f, 0.1f, 0.9f, 0.1f, 0.1f, 0.8f, 0.1f, 0.5f }, 0.15f)
            .route (Src::Motion1, Dst::Fx2Mix, 0.5f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.72f, 0.55f)
            .fx (FX::Delay, 0.06f, 0.24f, 0.34f, 0.88f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.25f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }
}
