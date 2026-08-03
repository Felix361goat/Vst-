#include "State/PresetBuilder.h"

namespace nog::presets
{
    // Acoustic instruments are approximations, not samples. What sells them is
    // the envelope shape and how the tone changes with velocity and pitch, far
    // more than the waveform underneath.
    std::vector<Preset> keysPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Soft Piano", "Keys")
            .voices (16)
            .osc (0, wt::BasicShapes, 0.30f, 0.75f)
            .osc (1, wt::OddEven, 0.55f, 0.25f)
            .tune (1, 1)
            .filter (flt::LP24, 1600.0f, 0.06f, 0.05f, 1.0f, 0.55f)
            // A piano's decay is long but its sustain never quite holds, which
            // is what the low sustain plus long decay imitates.
            .env (0, 1.0f, 2400.0f, 0.18f, 500.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 400.0f, 0.0f, 300.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.28f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.35f)
            .velocity (0.85f)
            .fx (FX::Reverb, 0.22f, 0.55f, 0.4f, 0.9f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Bright Piano", "Keys")
            .voices (16)
            .osc (0, wt::HarmonicSweep, 0.55f, 0.7f)
            .osc (1, wt::FmBell, 0.22f, 0.22f)
            .tune (1, 1)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.08f, 1.0f, 0.6f)
            .env (0, 0.5f, 2000.0f, 0.15f, 450.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 300.0f, 0.0f, 250.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.4f)
            .velocity (0.9f)
            .fx (FX::Eq, 1.0f, 0.48f, 0.62f, 0.5f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.35f, 0.95f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Afro Piano", "Keys")
            .voices (16)
            .osc (0, wt::BasicShapes, 0.28f, 0.78f)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (1, 1)
            .filter (flt::LP24, 1900.0f, 0.08f, 0.06f, 1.0f, 0.55f)
            .env (0, 1.0f, 1400.0f, 0.12f, 420.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 260.0f, 0.0f, 220.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.32f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.32f)
            .fx (FX::Delay, 0.14f, 0.22f, 0.24f, 0.7f)
            .fx (FX::Reverb, 0.26f, 0.62f, 0.35f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Synth E-Piano", "Keys")
            .voices (16)
            .osc (0, wt::FmBell, 0.30f, 0.7f)
            .osc (1, wt::BasicShapes, wt::sine, 0.35f)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 1.0f, 1800.0f, 0.22f, 500.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 350.0f, 0.0f, 300.0f)
            // Hitting an electric piano harder brings out the bark, not just
            // the level, so velocity drives the FM index too.
            .route (Src::Velocity, Dst::Osc1WtPos, 0.30f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.15f)
            .velocity (0.8f)
            .fx (FX::Chorus, 0.25f, 0.2f, 0.35f, 0.15f)
            .fx (FX::Reverb, 0.2f, 0.5f, 0.4f, 0.9f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Warm Rhodes", "Keys")
            .voices (16)
            .osc (0, wt::FmBell, 0.20f, 0.75f)
            .osc (1, wt::BasicShapes, wt::sine, 0.4f)
            .tune (1, 1)
            .filter (flt::LP24, 1800.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 2.0f, 2200.0f, 0.28f, 600.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Velocity, Dst::Osc1WtPos, 0.22f)
            .lfoFree (0, 0, 4.2f)
            .route (Src::Lfo1, Dst::Osc1Level, 0.06f, true)
            .fx (FX::Chorus, 0.3f, 0.15f, 0.4f, 0.2f)
            .fx (FX::Reverb, 0.24f, 0.58f, 0.38f, 0.95f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Drawbar Organ", "Keys")
            .voices (16)
            .osc (0, wt::Organ, 0.55f, 0.85f)
            .oscOff (1)
            .filter (flt::LP12, 7000.0f, 0.03f)
            .env (0, 3.0f, 300.0f, 1.0f, 90.0f)
            .fx (FX::Chorus, 0.4f, 0.6f, 0.55f, 0.2f)
            .fx (FX::Reverb, 0.18f, 0.5f, 0.4f, 0.9f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Church Organ", "Keys")
            .voices (16)
            .osc (0, wt::Organ, 0.85f, 0.8f)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (1, 1)
            .filter (flt::LP24, 5000.0f, 0.04f)
            .env (0, 60.0f, 400.0f, 1.0f, 320.0f)
            .fx (FX::Reverb, 0.45f, 0.92f, 0.25f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Toy Piano", "Keys")
            .voices (12)
            .osc (0, wt::FmBell, 0.55f, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 3000.0f, 0.1f, 0.0f, 1.0f, 0.7f)
            .env (0, 0.5f, 900.0f, 0.0f, 400.0f, 0.0f, 0.0f, 0.55f)
            .route (Src::Velocity, Dst::Osc1WtPos, 0.25f)
            .fx (FX::Reverb, 0.28f, 0.6f, 0.35f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Clav Funk", "Keys")
            .voices (12)
            .osc (0, wt::PulseWidth, 0.4f, 0.8f)
            .oscOff (1)
            .filter (flt::BP12, 1400.0f, 0.35f, 0.2f, 1.0f, 0.5f)
            .env (0, 0.5f, 500.0f, 0.1f, 200.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 120.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.3f)
            .fx (FX::Phaser, 0.25f, 0.2f, 0.5f, 0.3f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Lo-Fi Keys", "Keys")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.3f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 1100.0f, 0.08f, 0.15f, 1.0f, 0.4f)
            .env (0, 3.0f, 1600.0f, 0.15f, 400.0f, 0.0f, 0.0f, 0.45f)
            .lfoFree (0, 6, 0.6f)
            // A slow random wander in pitch is what reads as worn tape.
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.004f, true)
            .fx (FX::BitCrusher, 0.2f, 0.2f, 0.25f, 0.5f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.35f, 0.4f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.45f, 0.9f)
            .master (-8.0f)
            .done());

        return list;
    }

    std::vector<Preset> guitarPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Nylon String", "Plucked")
            .voices (12)
            .osc (0, wt::OddEven, 0.60f, 0.72f)
            .osc (1, wt::BasicShapes, 0.35f, 0.30f)
            .filter (flt::LP24, 1500.0f, 0.14f, 0.06f, 1.0f, 0.5f)
            // The pick noise is a short burst of filtered noise, not part of
            // the oscillator, which is why it does not follow the pitch.
            .noise (0, 0.10f)
            .env (0, 1.0f, 1400.0f, 0.0f, 420.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 60.0f, 0.0f, 60.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.35f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.30f)
            .fx (FX::Reverb, 0.22f, 0.55f, 0.4f, 0.9f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Steel String", "Plucked")
            .voices (12)
            .osc (0, wt::HarmonicSweep, 0.62f, 0.7f)
            .osc (1, wt::OddEven, 0.5f, 0.28f)
            .filter (flt::LP24, 2200.0f, 0.16f, 0.08f, 1.0f, 0.55f)
            .noise (0, 0.12f)
            .env (0, 0.5f, 1300.0f, 0.0f, 380.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 50.0f, 0.0f, 50.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.4f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.35f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.62f, 0.5f)
            .fx (FX::Reverb, 0.24f, 0.58f, 0.35f, 0.95f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Afro String Pluck", "Plucked")
            .voices (12)
            .osc (0, wt::OddEven, 0.55f, 0.72f)
            .osc (1, wt::BasicShapes, wt::sine, 0.25f)
            .tune (1, 1)
            .filter (flt::LP24, 1700.0f, 0.18f, 0.08f, 1.0f, 0.6f)
            .noise (0, 0.08f)
            .env (0, 0.5f, 620.0f, 0.0f, 320.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 45.0f, 0.0f, 45.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.35f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.3f)
            .fx (FX::Delay, 0.18f, 0.20f, 0.26f, 0.8f)
            .fx (FX::Reverb, 0.24f, 0.58f, 0.34f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Muted String", "Plucked")
            .voices (12)
            .osc (0, wt::OddEven, 0.45f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 900.0f, 0.2f, 0.12f, 1.0f, 0.45f)
            .noise (0, 0.1f)
            .env (0, 0.5f, 260.0f, 0.0f, 150.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 40.0f, 0.0f, 40.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.4f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.3f)
            .fx (FX::Reverb, 0.16f, 0.45f, 0.45f, 0.85f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Clean Electric String", "Plucked")
            .voices (12)
            .osc (0, wt::OddEven, 0.7f, 0.7f)
            .osc (1, wt::BasicShapes, 0.4f, 0.25f)
            .filter (flt::LP24, 2000.0f, 0.2f, 0.1f, 1.0f, 0.5f)
            .env (0, 1.0f, 1600.0f, 0.1f, 400.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 100.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.35f)
            .fx (FX::Chorus, 0.24f, 0.25f, 0.35f, 0.2f)
            .fx (FX::Reverb, 0.22f, 0.55f, 0.38f, 0.95f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Overdrive String", "Plucked")
            .voices (8)
            .osc (0, wt::OddEven, 0.75f, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 2400.0f, 0.25f, 0.3f, 1.0f, 0.5f)
            .env (0, 2.0f, 1600.0f, 0.55f, 400.0f)
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 800.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.007f, true)
            .fx (FX::Distortion, 0.65f, 0.5f, 0.45f, 0.5f)
            .fx (FX::Reverb, 0.2f, 0.5f, 0.4f, 0.9f)
            .oversample (1)
            .master (-10.0f)
            .done());

        list.push_back (Build ("String Harmonics", "Plucked")
            .voices (12)
            .osc (0, wt::FmBell, 0.32f, 0.6f)
            .osc (1, wt::OddEven, 0.6f, 0.35f)
            .tune (0, 1)
            .filter (flt::LP24, 3000.0f, 0.12f, 0.0f, 1.0f, 0.65f)
            .env (0, 1.0f, 1200.0f, 0.0f, 500.0f, 0.0f, 0.0f, 0.55f)
            .fx (FX::Delay, 0.2f, 0.26f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Twelve String", "Plucked")
            .voices (12)
            .osc (0, wt::OddEven, 0.6f, 0.6f)
            .osc (1, wt::OddEven, 0.6f, 0.4f)
            .tune (1, 1, 0, 8.0f)
            .unison (0, 3, 0.07f, 0.5f, 0.7f)
            .unison (1, 2, 0.05f, 0.5f, 0.9f)
            .filter (flt::LP24, 2000.0f, 0.15f, 0.08f, 1.0f, 0.55f)
            .noise (0, 0.08f)
            .env (0, 0.5f, 1500.0f, 0.0f, 450.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 50.0f, 0.0f, 50.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.35f)
            .fx (FX::Chorus, 0.2f, 0.2f, 0.3f, 0.15f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.35f, 1.0f)
            .master (-9.0f)
            .done());

        return list;
    }

    std::vector<Preset> bellPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Synth Music Box", "Bell")
            .voices (12)
            .osc (0, wt::FmBell, 0.48f, 0.75f)
            .oscOff (1)
            .tune (0, 1)
            .filter (flt::LP24, 4000.0f, 0.06f, 0.0f, 1.0f, 0.8f)
            .env (0, 0.5f, 1100.0f, 0.0f, 700.0f, 0.0f, 0.0f, 0.6f)
            .env (1, 0.5f, 250.0f, 0.0f, 200.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.25f)
            .fx (FX::Delay, 0.22f, 0.24f, 0.3f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.25f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Tubular Bell", "Bell")
            .voices (10)
            .osc (0, wt::FmBell, 0.68f, 0.7f)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (1, 1, 7)
            .filter (flt::LP24, 3400.0f, 0.08f, 0.0f, 1.0f, 0.7f)
            .env (0, 0.5f, 3200.0f, 0.0f, 1600.0f, 0.0f, 0.0f, 0.6f)
            .env (1, 0.5f, 400.0f, 0.0f, 350.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.3f)
            .fx (FX::Reverb, 0.42f, 0.88f, 0.2f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Glass Mallet", "Bell")
            .voices (12)
            .osc (0, wt::FmBell, 0.35f, 0.7f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.3f)
            .tune (1, 1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.85f)
            .env (0, 0.5f, 700.0f, 0.0f, 500.0f, 0.0f, 0.0f, 0.55f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.7f, 0.6f)
            .fx (FX::Reverb, 0.34f, 0.72f, 0.25f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Synth Kalimba", "Bell")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.22f, 0.8f)
            .osc (1, wt::FmBell, 0.3f, 0.25f)
            .filter (flt::LP24, 2200.0f, 0.12f, 0.0f, 1.0f, 0.6f)
            .env (0, 0.5f, 620.0f, 0.0f, 400.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 70.0f, 0.0f, 60.0f)
            .route (Src::Env2, Dst::Osc2Level, 0.3f)
            .fx (FX::Delay, 0.16f, 0.22f, 0.26f, 0.75f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.35f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Steel Drum", "Bell")
            .voices (10)
            .osc (0, wt::FmBell, 0.42f, 0.7f)
            .osc (1, wt::BasicShapes, 0.2f, 0.35f)
            .filter (flt::LP24, 2600.0f, 0.18f, 0.1f, 1.0f, 0.6f)
            .env (0, 1.0f, 800.0f, 0.05f, 450.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 120.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.3f)
            .route (Src::Velocity, Dst::Osc1WtPos, 0.2f)
            .fx (FX::Reverb, 0.3f, 0.65f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Celeste", "Bell")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::sine, 0.6f)
            .osc (1, wt::FmBell, 0.5f, 0.4f)
            .tune (1, 1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.8f)
            .env (0, 0.5f, 1400.0f, 0.0f, 800.0f, 0.0f, 0.0f, 0.6f)
            .fx (FX::Reverb, 0.38f, 0.8f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Dark Bell", "Bell")
            .voices (10)
            .osc (0, wt::FmBell, 0.75f, 0.65f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 1400.0f, 0.15f, 0.1f, 1.0f, 0.5f)
            .env (0, 1.0f, 2600.0f, 0.0f, 1400.0f, 0.0f, 0.0f, 0.55f)
            .lfoFree (0, 0, 0.25f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.1f, true)
            .fx (FX::Reverb, 0.44f, 0.9f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Sparkle Chime", "Bell")
            .voices (12)
            .osc (0, wt::FmBell, 0.38f, 0.6f)
            .osc (1, wt::HarmonicSweep, 0.9f, 0.25f)
            .tune (0, 1)
            .tune (1, 2)
            .filter (flt::LP24, 7000.0f, 0.04f, 0.0f, 1.0f, 0.9f)
            .env (0, 0.5f, 900.0f, 0.0f, 700.0f, 0.0f, 0.0f, 0.6f)
            .fx (FX::Eq, 1.0f, 0.40f, 0.78f, 0.65f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.34f, 0.95f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.20f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }
}
