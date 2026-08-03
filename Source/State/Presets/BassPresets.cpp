#include "State/PresetBuilder.h"

namespace nog::presets
{
    std::vector<Preset> bassPresets()
    {
        std::vector<Preset> list;

        // Sub bass is mono on purpose: two sub notes at once cancel as often as
        // they reinforce, and the low end turns to mush.
        list.push_back (Build ("Sub Sine", "Bass")
            .voices (1, 1)
            .osc (0, wt::BasicShapes, wt::sine, 0.9f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 900.0f, 0.0f)
            .env (0, 6.0f, 400.0f, 1.0f, 90.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("808 Long", "Bass")
            .voices (1, 1)
            .glide (55.0f, 2)
            .osc (0, wt::BasicShapes, wt::sine, 1.0f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 700.0f, 0.05f, 0.18f)
            // The long fall in pitch is what makes an 808 read as an 808.
            .env (0, 2.0f, 2600.0f, 0.0f, 320.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 1.0f, 260.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::Osc1Pitch, 0.10f)
            .fx (FX::Distortion, 0.35f, 0.30f, 0.45f, 0.5f)
            .oversample (1)
            .master (-7.0f)
            .done());

        list.push_back (Build ("808 Short Punch", "Bass")
            .voices (1, 1)
            .osc (0, wt::BasicShapes, wt::sine, 1.0f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 800.0f, 0.0f, 0.25f)
            .env (0, 1.0f, 700.0f, 0.0f, 120.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 0.5f, 90.0f, 0.0f, 60.0f)
            .route (Src::Env2, Dst::Osc1Pitch, 0.14f)
            .fx (FX::Distortion, 0.45f, 0.38f, 0.5f, 0.5f)
            .oversample (1)
            .master (-7.0f)
            .done());

        // Two saws a few cents apart is the whole trick; the beating between
        // them is the sound.
        list.push_back (Build ("Reese Classic", "Bass")
            .voices (2, 1)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::BasicShapes, wt::saw, 0.55f)
            .tune (0, -1, 0, -11.0f)
            .tune (1, -1, 0, 11.0f)
            .unison (0, 2, 0.08f, 0.5f, 0.4f)
            .unison (1, 2, 0.08f, 0.5f, 0.4f)
            .sub (0, 0.45f)
            .filter (flt::LP24, 420.0f, 0.22f, 0.15f)
            .env (0, 8.0f, 900.0f, 0.85f, 200.0f)
            .env (1, 60.0f, 700.0f, 0.35f, 300.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.16f)
            .lfoFree (0, 0, 0.18f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.06f, true)
            .fx (FX::Chorus, 0.30f, 0.14f, 0.4f, 0.25f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Dirty Reese", "Bass")
            .voices (2, 1)
            .osc (0, wt::HarmonicSweep, 0.72f, 0.55f)
            .osc (1, wt::BasicShapes, wt::saw, 0.5f)
            .tune (0, -1, 0, -14.0f)
            .tune (1, -1, 0, 14.0f)
            .sub (0, 0.4f)
            .filter (flt::LP24, 520.0f, 0.30f, 0.4f)
            .env (0, 5.0f, 800.0f, 0.9f, 180.0f)
            .lfoFree (0, 0, 0.22f)
            .route (Src::Lfo1, Dst::Osc1Detune, 0.20f, true)
            .fx (FX::Distortion, 0.5f, 0.42f, 0.4f, 0.5f)
            .fx (FX::Flanger, 0.22f, 0.08f, 0.5f, 0.55f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Growl Talker", "Bass")
            .voices (2, 1)
            .osc (0, wt::Growl, 0.25f, 0.8f)
            .oscOff (1)
            .tune (0, -1)
            .sub (0, 0.35f)
            .filter (flt::LP24, 900.0f, 0.35f, 0.3f)
            .env (0, 5.0f, 700.0f, 0.9f, 150.0f)
            // Sweeping the comb through the table is what makes it talk.
            .lfoSynced (0, 1, 6)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.5f)
            .route (Src::ModWheel, Dst::Osc1WtPos, 0.35f)
            .fx (FX::Distortion, 0.4f, 0.35f, 0.55f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Wobble Eighths", "Bass")
            .voices (2, 1)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .oscOff (1)
            .tune (0, -1)
            .sub (0, 0.5f)
            .filter (flt::LP24, 220.0f, 0.55f, 0.25f)
            .env (0, 4.0f, 600.0f, 1.0f, 140.0f)
            .lfoSynced (0, 0, 6)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.42f)
            .fx (FX::Distortion, 0.3f, 0.28f, 0.5f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Neuro Grind", "Bass")
            .voices (2, 1)
            .osc (0, wt::Digital, 0.4f, 0.6f)
            .osc (1, wt::Growl, 0.6f, 0.45f)
            .tune (0, -1)
            .tune (1, -1, 0, 7.0f)
            .sub (0, 0.4f)
            .filter (flt::BP12, 700.0f, 0.5f, 0.45f)
            .env (0, 3.0f, 700.0f, 0.9f, 130.0f)
            .lfoSynced (0, 5, 7)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.35f, true)
            .route (Src::Lfo1, Dst::Osc2WtPos, 0.4f)
            .fx (FX::BitCrusher, 0.28f, 0.35f, 0.42f, 0.5f)
            .fx (FX::Distortion, 0.45f, 0.45f, 0.45f, 0.5f)
            .oversample (2)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Rubber Pluck Bass", "Bass")
            .voices (4, 1)
            .osc (0, wt::BasicShapes, wt::square, 0.7f)
            .oscOff (1)
            .tune (0, -1)
            .sub (0, 0.35f)
            .filter (flt::LP24, 180.0f, 0.42f, 0.2f, 1.0f, 0.35f)
            .env (0, 1.0f, 420.0f, 0.25f, 120.0f)
            .env (1, 0.5f, 130.0f, 0.0f, 90.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.42f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.12f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Warm House Bass", "Bass")
            .voices (4, 1)
            .osc (0, wt::Bass, 0.35f, 0.85f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 480.0f, 0.12f, 0.1f, 1.0f, 0.3f)
            .env (0, 6.0f, 500.0f, 0.7f, 160.0f)
            .env (1, 3.0f, 220.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.22f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.15f)
            .fx (FX::Eq, 1.0f, 0.62f, 0.42f, 0.35f)
            .master (-6.0f)
            .done());

        // Afroswing and afrobeats basses sit low and round with almost no bite,
        // so the vocal and the plucks own everything above 200 Hz.
        list.push_back (Build ("Afro Round Bass", "Bass")
            .voices (2, 1)
            .glide (40.0f, 2)
            .osc (0, wt::BasicShapes, 0.12f, 0.9f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 380.0f, 0.08f)
            .env (0, 8.0f, 900.0f, 0.8f, 180.0f)
            .fx (FX::Eq, 1.0f, 0.6f, 0.4f, 0.3f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("FM Sub Knock", "Bass")
            .voices (2, 1)
            .osc (0, wt::FmBell, 0.22f, 0.8f)
            .oscOff (1)
            .tune (0, -1)
            .sub (0, 0.5f)
            .filter (flt::LP24, 600.0f, 0.1f, 0.2f)
            .env (0, 1.0f, 550.0f, 0.15f, 130.0f)
            .env (1, 0.5f, 70.0f, 0.0f, 50.0f)
            // A quick blip of extra FM index reads as the attack transient.
            .route (Src::Env2, Dst::Osc1WtPos, 0.45f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Hard Square Bass", "Bass")
            .voices (2, 1)
            .osc (0, wt::PulseWidth, 0.3f, 0.75f)
            .oscOff (1)
            .tune (0, -1)
            .sub (3, 0.4f)
            .filter (flt::LP24, 700.0f, 0.25f, 0.4f)
            .env (0, 2.0f, 400.0f, 0.85f, 100.0f)
            .lfoFree (0, 0, 0.5f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.18f, true)
            .fx (FX::Distortion, 0.5f, 0.4f, 0.45f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Offbeat Trance Bass", "Bass")
            .voices (4, 0)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .oscOff (1)
            .tune (0, -1)
            .unison (0, 2, 0.06f, 0.5f, 0.25f)
            .filter (flt::LP24, 320.0f, 0.35f, 0.15f, 1.0f, 0.25f)
            .env (0, 2.0f, 260.0f, 0.0f, 90.0f)
            .env (1, 1.0f, 150.0f, 0.0f, 80.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .fx (FX::Chorus, 0.2f, 0.25f, 0.3f, 0.2f)
            .master (-7.0f)
            .done());

        return list;
    }
}
