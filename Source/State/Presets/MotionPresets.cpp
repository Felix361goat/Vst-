#include "State/PresetBuilder.h"

namespace nog::presets
{
    /**
        Patches whose movement is the point.

        Each of these holds a chord and lets a tempo-locked step pattern do the
        rest: a gate on the effect mix, a filter that moves in sixteenths, a
        pan that hops between beats. Playing one note gets you a part rather
        than a sound, which is the difference between a preset you audition and
        one you build a bar out of.

        This is what Motion is for, and none of it was possible before, because
        the effects rack runs once on the summed output and every LFO in the
        synth belongs to a voice.
    */
    std::vector<Preset> motionPresets()
    {
        std::vector<Preset> list;

        // -- gated ----------------------------------------------------------

        // The trance gate: a sustained chord chopped into a rhythm by the
        // amplitude alone. Nothing else about the patch moves.
        list.push_back (Build ("Gated Trance Chords", "Trance")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.62f)
            .osc (1, wt::BasicShapes, wt::saw, 0.34f)
            .tune (1, 0, 0, 9.0f)
            .unison (0, 7, 0.16f, 0.58f, 0.95f, 0.12f)
            .filter (flt::LP24, 3000.0f, 0.12f, 0.08f, 1.0f, 0.6f)
            .env (0, 3.0f, 900.0f, 0.85f, 260.0f)
            .motion (0, 7, { 1.0f, 0.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.5f, 0.3f }, 0.06f)
            .route (Src::Motion1, Dst::MasterGain, 0.55f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.55f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Gated Sparkle Chords", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.52f)
            .osc (1, wt::HarmonicSweep, 0.78f, 0.26f)
            .tune (1, 1, 0, 5.0f)
            .unison (0, 7, 0.14f, 0.55f, 0.9f, 0.10f)
            .filter (flt::LP24, 2600.0f, 0.10f, 0.06f, 1.0f, 0.7f)
            .env (0, 12.0f, 1400.0f, 0.75f, 620.0f)
            // Smoothed, so the gate breathes rather than clicks.
            .motion (0, 7, { 1.0f, 0.25f, 0.8f, 0.25f, 1.0f, 0.4f, 0.6f, 0.2f }, 0.35f)
            .route (Src::Motion1, Dst::MasterGain, 0.45f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.70f, 0.55f)
            .fx (FX::Delay, 0.22f, 0.30f, 0.32f, 0.8f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // -- rhythmic filter ------------------------------------------------

        list.push_back (Build ("Motion Filter Pad", "Pad")
            .voices (10)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::Formant, 0.32f, 0.28f)
            .tune (1, 0)
            .unison (0, 5, 0.13f, 0.55f, 0.9f, 0.2f)
            .filter (flt::LP24, 700.0f, 0.28f, 0.10f, 1.0f, 0.35f)
            .env (0, 300.0f, 2200.0f, 0.85f, 1200.0f)
            .motion (0, 7, { 1.0f, 0.15f, 0.55f, 0.15f, 0.85f, 0.3f, 0.4f, 0.15f }, 0.25f)
            .route (Src::Motion1, Dst::FilterCutoff, 0.45f)
            .fx (FX::Chorus, 0.24f, 0.10f, 0.34f, 0.28f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        // Two patterns running against each other at different rates: the
        // filter in sixteenths and the delay send in eighths, so the part never
        // repeats the same way twice within a bar.
        list.push_back (Build ("Cross Motion Keys", "Keys")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.40f, 0.55f)
            .osc (1, wt::FmBell, 0.30f, 0.24f)
            .tune (1, 1)
            .filter (flt::LP24, 1600.0f, 0.20f, 0.08f, 1.0f, 0.55f)
            .env (0, 2.0f, 1600.0f, 0.55f, 500.0f)
            .motion (0, 7, { 1.0f, 0.3f, 0.7f, 0.2f, 0.9f, 0.35f, 0.6f, 0.25f }, 0.2f)
            .motion (1, 6, { 0.0f, 0.8f, 0.2f, 1.0f, 0.1f, 0.6f, 0.3f, 0.9f }, 0.4f)
            .route (Src::Motion1, Dst::FilterCutoff, 0.35f)
            .route (Src::Motion2, Dst::Fx2Mix, 0.5f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.62f, 0.5f)
            .fx (FX::Delay, 0.10f, 0.28f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.76f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // -- rhythmic effects -----------------------------------------------

        // The crusher is switched in and out by the pattern rather than sitting
        // on the whole part, which is what stops it becoming tiring.
        list.push_back (Build ("Motion Crush Lead", "Lead")
            .voices (8)
            .osc (0, wt::BasicShapes, wt::saw, 0.60f)
            .osc (1, wt::Digital, 0.30f, 0.24f)
            .tune (1, 1)
            .unison (0, 5, 0.14f, 0.55f, 0.88f, 0.15f)
            .filter (flt::LP24, 3000.0f, 0.12f, 0.10f, 1.0f, 0.75f)
            .env (0, 3.0f, 900.0f, 0.75f, 380.0f)
            .motion (0, 8, { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.6f })
            .route (Src::Motion1, Dst::Fx1Mix, 0.85f)
            .fx (FX::BitCrusher, 0.0f, 0.45f, 0.35f, 0.5f)
            .fx (FX::Delay, 0.24f, 0.26f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.30f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Motion Stutter Pluck", "Pluck")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.60f)
            .osc (1, wt::FmBell, 0.34f, 0.22f)
            .tune (1, 1)
            .filter (flt::LP24, 1700.0f, 0.18f, 0.06f, 1.0f, 0.7f)
            .env (0, 1.0f, 650.0f, 0.06f, 340.0f)
            .env (1, 0.5f, 200.0f, 0.0f, 160.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.40f)
            // The delay feedback jumps on two steps out of eight, which reads
            // as a stutter without needing a stutter effect.
            .motion (0, 7, { 0.2f, 0.2f, 0.2f, 0.95f, 0.2f, 0.2f, 0.9f, 0.2f }, 0.05f)
            .route (Src::Motion1, Dst::Fx2C, 0.5f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.5f)
            .fx (FX::Delay, 0.30f, 0.12f, 0.30f, 0.4f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Motion Pan Bells", "Bell")
            .voices (10)
            .osc (0, wt::FmBell, 0.40f, 0.62f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.26f)
            .tune (1, 1)
            .filter (flt::LP24, 5000.0f, 0.06f, 0.0f, 1.0f, 0.7f)
            .env (0, 1.0f, 1800.0f, 0.20f, 700.0f)
            // A hop rather than a sweep: hard left, hard right, and two stops
            // in between, which is far more interesting than an auto-pan.
            .motion (0, 6, { 0.0f, 1.0f, 0.3f, 0.8f, 0.1f, 0.9f, 0.4f, 0.6f }, 0.15f)
            .route (Src::Motion1, Dst::Osc1Pan, 0.8f, true)
            .fx (FX::Delay, 0.26f, 0.24f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Motion House Stab", "House")
            .voices (10)
            .osc (0, wt::BasicShapes, wt::saw, 0.65f)
            .oscOff (1)
            .unison (0, 5, 0.12f, 0.55f, 0.85f)
            .filter (flt::LP24, 1400.0f, 0.26f, 0.14f, 1.0f, 0.6f)
            .env (0, 2.0f, 420.0f, 0.0f, 200.0f)
            .motion (0, 7, { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f }, 0.04f)
            .route (Src::Motion1, Dst::MasterGain, 0.6f)
            .fx (FX::Distortion, 0.18f, 0.22f, 0.5f, 0.5f)
            .fx (FX::Delay, 0.20f, 0.24f, 0.30f, 0.8f)
            .fx (FX::Reverb, 0.26f, 0.68f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Motion Width Pad", "Pad")
            .voices (10)
            .osc (0, wt::BasicShapes, 0.44f, 0.55f)
            .osc (1, wt::Formant, 0.30f, 0.26f)
            .tune (1, 0)
            .unison (0, 5, 0.12f, 0.55f, 0.9f, 0.3f)
            .filter (flt::LP24, 2200.0f, 0.06f, 0.0f, 1.0f, 0.4f)
            .env (0, 500.0f, 2400.0f, 0.85f, 1600.0f)
            // The widener opens and closes with the bar, so the pad breathes
            // outwards instead of sitting at one width.
            .motion (0, 5, { 0.2f, 0.7f, 0.4f, 1.0f, 0.3f, 0.8f, 0.5f, 0.9f }, 0.6f)
            .route (Src::Motion1, Dst::Fx1A, 0.7f)
            .fx (FX::Dimension, 0.55f, 0.30f, 0.25f, 1.0f)
            .fx (FX::Reverb, 0.48f, 0.88f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Motion Drill Bell", "Drill")
            .voices (10)
            .arp (0, 7, 2, 0.4f, 0.0f)
            .osc (0, wt::FmBell, 0.44f, 0.65f)
            .oscOff (1)
            .tune (0, 1)
            .filter (flt::LP24, 4200.0f, 0.08f, 0.05f, 1.0f, 0.7f)
            .env (0, 0.5f, 700.0f, 0.0f, 260.0f)
            .motion (0, 6, { 1.0f, 0.4f, 0.8f, 0.2f, 1.0f, 0.5f, 0.7f, 0.3f }, 0.2f)
            .route (Src::Motion1, Dst::FilterCutoff, 0.30f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }
}
