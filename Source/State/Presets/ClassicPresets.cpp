#include "State/PresetBuilder.h"

namespace nog::presets
{
    std::vector<Preset> arpPresets()
    {
        std::vector<Preset> list;

        // Built for fast note runs: very short envelopes, no unison smear, and
        // enough release to let the tail of one note reach the next.
        list.push_back (Build ("Arp Bright", "Arp")
            .arp (0, 7, 2, 0.55f, 0.00f)
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 1600.0f, 0.22f, 0.1f, 1.0f, 0.6f)
            .env (0, 0.5f, 180.0f, 0.0f, 160.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 100.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Arp Bell", "Arp")
            .arp (2, 7, 2, 0.60f, 0.00f)
            .voices (12)
            .osc (0, wt::FmBell, 0.38f, 0.7f)
            .oscOff (1)
            .tune (0, 1)
            .filter (flt::LP24, 4000.0f, 0.06f, 0.0f, 1.0f, 0.8f)
            .env (0, 0.5f, 300.0f, 0.0f, 280.0f, 0.0f, 0.0f, 0.55f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.38f, 0.95f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Arp Digital", "Arp")
            .arp (0, 8, 1, 0.45f, 0.00f)
            .voices (12)
            .osc (0, wt::Digital, 0.35f, 0.7f)
            .oscOff (1)
            .filter (flt::BP12, 1800.0f, 0.32f, 0.1f, 1.0f, 0.55f)
            .env (0, 0.5f, 150.0f, 0.0f, 130.0f, 0.0f, 0.0f, 0.5f)
            .lfoSynced (0, 5, 7)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.3f, true)
            .fx (FX::Delay, 0.3f, 0.16f, 0.4f, 0.95f)
            .fx (FX::Reverb, 0.28f, 0.65f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Arp Pulse", "Arp")
            .arp (3, 7, 2, 0.50f, 0.12f)
            .voices (12)
            .osc (0, wt::PulseWidth, 0.35f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 2000.0f, 0.2f, 0.1f, 1.0f, 0.55f)
            .env (0, 0.5f, 200.0f, 0.0f, 170.0f, 0.0f, 0.0f, 0.5f)
            .lfoFree (0, 0, 0.6f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.3f, true)
            .fx (FX::Delay, 0.26f, 0.18f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Arp Soft Sine", "Arp")
            .arp (2, 6, 2, 0.70f, 0.00f)
            .voices (12)
            .osc (0, wt::BasicShapes, wt::sine, 0.85f)
            .oscOff (1)
            .filter (flt::LP12, 5000.0f, 0.04f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 260.0f, 0.0f, 240.0f, 0.0f, 0.0f, 0.5f)
            .fx (FX::Delay, 0.3f, 0.18f, 0.4f, 0.95f)
            .fx (FX::Reverb, 0.34f, 0.75f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Arp Hard Sync", "Arp")
            .arp (0, 7, 1, 0.45f, 0.00f)
            .voices (12)
            .osc (0, wt::HardSync, 0.35f, 0.72f)
            .oscOff (1)
            .filter (flt::LP24, 1900.0f, 0.25f, 0.18f, 1.0f, 0.55f)
            .env (0, 0.5f, 190.0f, 0.0f, 160.0f, 0.0f, 0.0f, 0.5f)
            .lfoSynced (0, 0, 3)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.35f, true)
            .fx (FX::Delay, 0.26f, 0.18f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.3f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }

    std::vector<Preset> synthwavePresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Retro Brass", "Synthwave")
            .voices (10)
            .osc (0, wt::BasicShapes, wt::saw, 0.65f)
            .osc (1, wt::BasicShapes, wt::saw, 0.45f)
            .tune (1, 0, 0, -9.0f)
            .unison (0, 3, 0.10f, 0.5f, 0.7f)
            .filter (flt::LP24, 1100.0f, 0.18f, 0.15f, 1.0f, 0.45f)
            // Brass gets its bite from the filter opening slightly after the
            // note starts, not from the waveform.
            .env (0, 45.0f, 900.0f, 0.85f, 260.0f)
            .env (1, 90.0f, 600.0f, 0.45f, 300.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.36f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.2f)
            .fx (FX::Chorus, 0.24f, 0.2f, 0.35f, 0.2f)
            .fx (FX::Reverb, 0.28f, 0.7f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Synthwave Lead", "Synthwave")
            .voices (8, 1)
            .glide (30.0f, 2)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .osc (1, wt::PulseWidth, 0.4f, 0.4f)
            .unison (0, 5, 0.16f, 0.55f, 0.85f)
            .filter (flt::LP24, 2400.0f, 0.2f, 0.2f, 1.0f, 0.55f)
            .env (0, 8.0f, 700.0f, 0.85f, 300.0f)
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 600.0f, 0.15f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.010f, true)
            .fx (FX::Chorus, 0.26f, 0.22f, 0.4f, 0.2f)
            .fx (FX::Delay, 0.26f, 0.28f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.34f, 0.8f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Synthwave Bass", "Synthwave")
            .voices (2, 1)
            .osc (0, wt::BasicShapes, wt::saw, 0.75f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 600.0f, 0.28f, 0.2f, 1.0f, 0.3f)
            .env (0, 3.0f, 500.0f, 0.7f, 150.0f)
            .env (1, 1.0f, 220.0f, 0.0f, 150.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .fx (FX::Distortion, 0.25f, 0.25f, 0.5f, 0.5f)
            .fx (FX::Chorus, 0.18f, 0.2f, 0.3f, 0.15f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Neon Pad", "Synthwave")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::PulseWidth, 0.45f, 0.4f)
            .unison (0, 7, 0.16f, 0.6f, 1.0f)
            .filter (flt::LP24, 1300.0f, 0.14f, 0.08f, 1.0f, 0.4f)
            .env (0, 600.0f, 1400.0f, 0.85f, 1200.0f)
            .lfoFree (0, 0, 0.16f)
            .route (Src::Lfo1, Dst::Osc2WtPos, 0.3f, true)
            .fx (FX::Chorus, 0.3f, 0.15f, 0.45f, 0.25f)
            .fx (FX::Reverb, 0.44f, 0.9f, 0.28f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Retro Poly Keys", "Synthwave")
            .voices (16)
            .osc (0, wt::PulseWidth, 0.4f, 0.7f)
            .osc (1, wt::BasicShapes, wt::saw, 0.35f)
            .tune (1, 0, 0, 7.0f)
            .filter (flt::LP24, 1600.0f, 0.15f, 0.1f, 1.0f, 0.5f)
            .env (0, 5.0f, 900.0f, 0.55f, 350.0f)
            .env (1, 2.0f, 350.0f, 0.2f, 250.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .lfoFree (0, 0, 0.4f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.25f, true)
            .fx (FX::Chorus, 0.28f, 0.2f, 0.4f, 0.2f)
            .fx (FX::Reverb, 0.3f, 0.75f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Outrun Arp", "Synthwave")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::square, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 1500.0f, 0.28f, 0.12f, 1.0f, 0.6f)
            .env (0, 0.5f, 220.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 120.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Chorus, 0.2f, 0.25f, 0.35f, 0.2f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.38f, 0.95f)
            .fx (FX::Reverb, 0.3f, 0.72f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }

    std::vector<Preset> orchestralPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Saw Strings", "Strings")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.6f)
            .osc (1, wt::OddEven, 0.65f, 0.35f)
            .unison (0, 7, 0.11f, 0.5f, 1.0f)
            .filter (flt::LP24, 1500.0f, 0.08f, 0.05f, 1.0f, 0.4f)
            // Strings swell rather than start, and the section is never quite
            // in tune with itself, which is what the slow vibrato imitates.
            .env (0, 320.0f, 1200.0f, 0.9f, 800.0f)
            .lfoFree (0, 0, 4.6f)
            .lfoShape (0, 1400.0f, 0.35f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.006f, true)
            .fx (FX::Reverb, 0.44f, 0.9f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Cinematic Strings", "Strings")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::BasicShapes, 0.45f, 0.4f)
            .tune (1, -1)
            .unison (0, 9, 0.13f, 0.55f, 1.0f)
            .filter (flt::LP24, 1200.0f, 0.08f, 0.05f, 1.0f, 0.35f)
            .env (0, 600.0f, 1600.0f, 0.9f, 1400.0f)
            .lfoFree (0, 0, 4.2f)
            .lfoShape (0, 1800.0f, 0.4f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Reverb, 0.52f, 0.95f, 0.28f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Pizzicato", "Strings")
            .voices (12)
            .osc (0, wt::OddEven, 0.5f, 0.75f)
            .oscOff (1)
            .noise (0, 0.10f)
            .filter (flt::LP24, 1900.0f, 0.2f, 0.08f, 1.0f, 0.55f)
            .env (0, 0.5f, 240.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 40.0f, 0.0f, 40.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.4f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.3f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.32f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Brass Section", "Brass")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.68f)
            .osc (1, wt::Formant, 0.2f, 0.3f)
            .unison (0, 5, 0.09f, 0.5f, 0.7f)
            .filter (flt::LP24, 1000.0f, 0.2f, 0.2f, 1.0f, 0.45f)
            .env (0, 60.0f, 800.0f, 0.85f, 240.0f)
            .env (1, 120.0f, 500.0f, 0.5f, 260.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.25f)
            .fx (FX::Reverb, 0.3f, 0.72f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Soft Horn", "Brass")
            .voices (10)
            .osc (0, wt::BasicShapes, 0.35f, 0.75f)
            .oscOff (1)
            .filter (flt::LP24, 900.0f, 0.12f, 0.12f, 1.0f, 0.4f)
            .env (0, 120.0f, 900.0f, 0.85f, 300.0f)
            .env (1, 200.0f, 600.0f, 0.5f, 300.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .lfoFree (0, 0, 4.4f)
            .lfoShape (0, 1200.0f, 0.3f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Reverb, 0.36f, 0.8f, 0.32f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Stab Brass", "Brass")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.72f)
            .oscOff (1)
            .unison (0, 5, 0.10f, 0.5f, 0.7f)
            .filter (flt::LP24, 800.0f, 0.28f, 0.25f, 1.0f, 0.45f)
            .env (0, 6.0f, 420.0f, 0.2f, 200.0f)
            .env (1, 3.0f, 200.0f, 0.0f, 160.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.5f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.25f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }

    std::vector<Preset> ambientPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Deep Space", "Ambient")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::sine, 0.5f)
            .osc (1, wt::FmBell, 0.2f, 0.3f)
            .tune (1, 1)
            .unison (0, 3, 0.09f, 0.5f, 1.0f)
            .filter (flt::LP24, 1200.0f, 0.08f)
            .env (0, 2000.0f, 2500.0f, 0.85f, 3000.0f)
            .lfoFree (0, 0, 0.06f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.2f, true)
            .lfoFree (1, 6, 0.04f)
            .route (Src::Lfo2, Dst::Osc2Level, 0.25f, true)
            .fx (FX::Delay, 0.28f, 0.6f, 0.4f, 1.0f)
            .fx (FX::Reverb, 0.6f, 0.98f, 0.2f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Frozen Texture", "Ambient")
            .voices (12)
            .osc (0, wt::Digital, 0.3f, 0.45f)
            .osc (1, wt::BasicShapes, wt::sine, 0.4f)
            .tune (0, 1)
            .noise (1, 0.12f)
            .filter (flt::BP12, 2000.0f, 0.25f)
            .env (0, 1500.0f, 2000.0f, 0.8f, 2500.0f)
            .lfoFree (0, 6, 0.12f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.35f, true)
            .fx (FX::Delay, 0.3f, 0.55f, 0.42f, 1.0f)
            .fx (FX::Reverb, 0.58f, 0.96f, 0.22f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Slow Bloom", "Ambient")
            .voices (12)
            .osc (0, wt::HarmonicSweep, 0.3f, 0.6f)
            .oscOff (1)
            .unison (0, 5, 0.12f, 0.55f, 1.0f)
            .filter (flt::LP24, 700.0f, 0.15f)
            .env (0, 2500.0f, 3000.0f, 0.8f, 3500.0f)
            .env (1, 3000.0f, 2000.0f, 0.6f, 3000.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.5f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .fx (FX::Reverb, 0.62f, 0.98f, 0.2f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Underwater", "Ambient")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.3f, 0.65f)
            .oscOff (1)
            .filter (flt::LP24, 600.0f, 0.3f)
            .env (0, 900.0f, 2000.0f, 0.8f, 2000.0f)
            .lfoFree (0, 0, 0.22f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.3f, true)
            .lfoFree (1, 0, 0.18f)
            .route (Src::Lfo2, Dst::Osc1Pitch, 0.010f, true)
            .fx (FX::Chorus, 0.35f, 0.1f, 0.5f, 0.3f)
            .fx (FX::Reverb, 0.55f, 0.95f, 0.35f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Drone Bed", "Ambient")
            .voices (8)
            .osc (0, wt::Growl, 0.5f, 0.55f)
            .osc (1, wt::Bass, 0.35f, 0.45f)
            .tune (0, -1)
            .tune (1, -2)
            .filter (flt::LP24, 500.0f, 0.15f, 0.1f)
            .env (0, 1800.0f, 2500.0f, 0.85f, 3000.0f)
            .lfoFree (0, 6, 0.05f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.35f, true)
            .fx (FX::Reverb, 0.6f, 0.98f, 0.3f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Shimmer Wash", "Ambient")
            .voices (12)
            .osc (0, wt::FmBell, 0.25f, 0.45f)
            .osc (1, wt::HarmonicSweep, 0.6f, 0.3f)
            .tune (0, 1)
            .tune (1, 2)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.6f)
            .env (0, 1400.0f, 2200.0f, 0.75f, 2800.0f)
            .lfoFree (0, 0, 0.09f)
            .route (Src::Lfo1, Dst::Osc2Level, 0.3f, true)
            .fx (FX::Eq, 1.0f, 0.4f, 0.72f, 0.6f)
            .fx (FX::Delay, 0.3f, 0.5f, 0.4f, 1.0f)
            .fx (FX::Reverb, 0.62f, 0.98f, 0.18f, 1.0f)
            .master (-12.0f)
            .done());

        return list;
    }

    std::vector<Preset> houseTechnoPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("House Organ Stab", "House")
            .voices (12)
            .osc (0, wt::Organ, 0.45f, 0.8f)
            .oscOff (1)
            .filter (flt::LP24, 1600.0f, 0.2f, 0.2f, 1.0f, 0.5f)
            .env (0, 1.0f, 300.0f, 0.0f, 200.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 140.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Distortion, 0.2f, 0.2f, 0.55f, 0.5f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.32f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Deep House Chord", "House")
            .voices (16)
            .osc (0, wt::BasicShapes, 0.38f, 0.6f)
            .osc (1, wt::Organ, 0.4f, 0.35f)
            .filter (flt::LP24, 1100.0f, 0.12f, 0.1f, 1.0f, 0.4f)
            .env (0, 12.0f, 700.0f, 0.35f, 400.0f)
            .fx (FX::Chorus, 0.24f, 0.18f, 0.4f, 0.2f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Acid Line", "House")
            .voices (2, 1)
            .glide (25.0f, 2)
            .osc (0, wt::BasicShapes, wt::saw, 0.8f)
            .oscOff (1)
            .tune (0, -1)
            // The whole sound is a resonant filter with a fast envelope on it.
            .filter (flt::LP24, 300.0f, 0.72f, 0.3f, 1.0f, 0.4f)
            .env (0, 1.0f, 400.0f, 0.6f, 120.0f)
            .env (1, 1.0f, 220.0f, 0.0f, 150.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.5f)
            .route (Src::ModWheel, Dst::FilterCutoff, 0.25f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.15f)
            .fx (FX::Distortion, 0.35f, 0.32f, 0.5f, 0.5f)
            .oversample (1)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Techno Stab", "House")
            .voices (10)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .osc (1, wt::Digital, 0.3f, 0.3f)
            .unison (0, 3, 0.14f, 0.5f, 0.8f)
            .filter (flt::BP12, 1200.0f, 0.4f, 0.3f)
            .env (0, 0.5f, 260.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 120.0f, 0.0f, 100.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Distortion, 0.35f, 0.35f, 0.45f, 0.5f)
            .fx (FX::Delay, 0.22f, 0.2f, 0.35f, 0.9f)
            .fx (FX::Reverb, 0.26f, 0.68f, 0.3f, 1.0f)
            .oversample (1)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Rolling Techno Bass", "House")
            .voices (2, 1)
            .osc (0, wt::Bass, 0.3f, 0.85f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 400.0f, 0.2f, 0.25f, 1.0f, 0.25f)
            .env (0, 1.0f, 260.0f, 0.0f, 90.0f)
            .env (1, 0.5f, 130.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.3f)
            .fx (FX::Distortion, 0.3f, 0.3f, 0.5f, 0.5f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Hypnotic Pluck", "House")
            .voices (12)
            .osc (0, wt::PulseWidth, 0.45f, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 1100.0f, 0.3f, 0.12f, 1.0f, 0.5f)
            .env (0, 0.5f, 240.0f, 0.0f, 200.0f, 0.0f, 0.0f, 0.5f)
            .lfoSynced (0, 0, 2)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.25f, true)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.25f, true)
            .fx (FX::Delay, 0.3f, 0.18f, 0.4f, 0.95f)
            .fx (FX::Reverb, 0.32f, 0.75f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        return list;
    }
}
