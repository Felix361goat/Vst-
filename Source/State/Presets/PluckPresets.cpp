#include "State/PresetBuilder.h"

namespace nog::presets
{
    std::vector<Preset> pluckPresets()
    {
        std::vector<Preset> list;

        // Afroswing and afrobeats plucks are short, bright and dry-ish, with
        // enough decay to ring into the next note but not enough to blur the
        // rhythm. Key tracking matters: the same patch has to stay crisp two
        // octaves up without turning into an ice pick.
        list.push_back (Build ("Afro Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, 0.44f, 0.75f)
            .osc (1, wt::BasicShapes, wt::sine, 0.30f)
            .tune (1, 1)
            .unison (0, 3, 0.09f, 0.5f, 0.55f)
            .filter (flt::LP24, 900.0f, 0.20f, 0.1f, 1.0f, 0.55f)
            .env (0, 1.0f, 340.0f, 0.0f, 260.0f, 0.0f, 0.0f, 0.35f)
            .env (1, 0.5f, 170.0f, 0.0f, 150.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.42f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.20f)
            .fx (FX::Delay, 0.18f, 0.20f, 0.28f, 0.8f)
            .fx (FX::Reverb, 0.22f, 0.55f, 0.35f, 0.9f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Afro Bright Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::HarmonicSweep, 0.70f, 0.72f)
            .osc (1, wt::FmBell, 0.28f, 0.26f)
            .tune (1, 1)
            .unison (0, 3, 0.11f, 0.5f, 0.7f)
            .filter (flt::LP24, 1400.0f, 0.18f, 0.12f, 1.0f, 0.7f)
            .env (0, 1.0f, 300.0f, 0.0f, 240.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 0.5f, 150.0f, 0.0f, 140.0f, 0.0f, 0.0f, 0.45f)
            .route (Src::Env2, Dst::FilterCutoff, 0.40f)
            .route (Src::Velocity, Dst::Osc2Level, 0.25f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.66f, 0.5f)
            .fx (FX::Delay, 0.20f, 0.22f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.32f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Afro Marimba Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, 0.20f, 0.8f)
            .osc (1, wt::FmBell, 0.35f, 0.22f)
            .tune (1, 1, 7)
            .filter (flt::LP24, 1900.0f, 0.10f, 0.0f, 1.0f, 0.6f)
            // Wooden mallets have almost no attack ramp and a fast, clean fall.
            .env (0, 0.5f, 420.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 90.0f, 0.0f, 80.0f)
            .route (Src::Env2, Dst::Osc2Level, 0.30f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.22f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.35f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Koto Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::OddEven, 0.55f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 1600.0f, 0.25f, 0.1f, 1.0f, 0.65f)
            .env (0, 0.5f, 600.0f, 0.0f, 400.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 120.0f, 0.0f, 110.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.25f)
            .fx (FX::Delay, 0.18f, 0.24f, 0.26f, 0.75f)
            .fx (FX::Reverb, 0.26f, 0.62f, 0.32f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Glass Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::FmBell, 0.40f, 0.7f)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (1, 1)
            .filter (flt::LP24, 3200.0f, 0.08f, 0.0f, 1.0f, 0.8f)
            .env (0, 0.5f, 520.0f, 0.0f, 420.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 200.0f, 0.0f, 180.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.28f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.7f, 0.6f)
            .fx (FX::Delay, 0.24f, 0.22f, 0.32f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.75f, 0.25f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Y2K Trance Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .oscOff (1)
            .unison (0, 5, 0.14f, 0.55f, 0.8f)
            .filter (flt::LP24, 700.0f, 0.30f, 0.1f, 1.0f, 0.5f)
            .env (0, 1.0f, 260.0f, 0.0f, 200.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 0.5f, 140.0f, 0.0f, 130.0f, 0.0f, 0.0f, 0.45f)
            .route (Src::Env2, Dst::FilterCutoff, 0.50f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.38f, 0.9f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Nylon Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, 0.32f, 0.75f)
            .osc (1, wt::OddEven, 0.4f, 0.25f)
            .filter (flt::LP24, 1500.0f, 0.12f, 0.08f, 1.0f, 0.5f)
            .env (0, 2.0f, 700.0f, 0.0f, 380.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 1.0f, 130.0f, 0.0f, 120.0f, 0.0f, 0.0f, 0.35f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.25f)
            .noise (0, 0.05f)
            .fx (FX::Reverb, 0.2f, 0.5f, 0.4f, 0.9f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Hollow Wood Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::PulseWidth, 0.55f, 0.7f)
            .oscOff (1)
            .filter (flt::BP12, 1100.0f, 0.28f, 0.1f, 1.0f, 0.5f)
            .env (0, 0.5f, 380.0f, 0.0f, 260.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 110.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Reverb, 0.24f, 0.55f, 0.35f, 0.95f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Dark Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::Bass, 0.45f, 0.8f)
            .oscOff (1)
            .filter (flt::LP24, 520.0f, 0.25f, 0.12f, 1.0f, 0.45f)
            .env (0, 1.0f, 420.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 0.5f, 180.0f, 0.0f, 160.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.34f)
            .fx (FX::Delay, 0.2f, 0.26f, 0.3f, 0.8f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.35f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Metal Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::Digital, 0.5f, 0.65f)
            .osc (1, wt::FmBell, 0.6f, 0.3f)
            .tune (1, 1, 7)
            .filter (flt::LP24, 2400.0f, 0.2f, 0.15f, 1.0f, 0.7f)
            .env (0, 0.5f, 360.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 130.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Delay, 0.24f, 0.2f, 0.35f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Soft Sine Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, wt::sine, 0.9f)
            .oscOff (1)
            .filter (flt::LP12, 4000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 500.0f, 0.0f, 380.0f, 0.0f, 0.0f, 0.5f)
            .fx (FX::Delay, 0.22f, 0.24f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.3f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Wide Stack Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::BasicShapes, wt::saw, 0.6f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.35f)
            .tune (1, 1)
            .unison (0, 7, 0.16f, 0.6f, 1.0f)
            .filter (flt::LP24, 1300.0f, 0.18f, 0.1f, 1.0f, 0.6f)
            .env (0, 2.0f, 420.0f, 0.0f, 340.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 1.0f, 190.0f, 0.0f, 170.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.42f)
            .fx (FX::Chorus, 0.2f, 0.3f, 0.35f, 0.2f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.3f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Formant Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::Formant, 0.4f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 1800.0f, 0.15f, 0.1f, 1.0f, 0.55f)
            .env (0, 1.0f, 400.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 220.0f, 0.0f, 180.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.35f)
            .route (Src::Velocity, Dst::Osc1WtPos, 0.2f)
            .fx (FX::Delay, 0.2f, 0.24f, 0.3f, 0.8f)
            .fx (FX::Reverb, 0.26f, 0.62f, 0.32f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Sync Stab Pluck", "Pluck")
            .voices (8)
            .osc (0, wt::HardSync, 0.4f, 0.72f)
            .oscOff (1)
            .unison (0, 3, 0.1f, 0.5f, 0.6f)
            .filter (flt::LP24, 1500.0f, 0.22f, 0.2f, 1.0f, 0.5f)
            .env (0, 0.5f, 320.0f, 0.0f, 240.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 160.0f, 0.0f, 140.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .fx (FX::Delay, 0.22f, 0.22f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.32f, 1.0f)
            .master (-9.0f)
            .done());

        return list;
    }
}
