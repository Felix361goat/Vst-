#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        // The non-instrument half of the sample bank: blips, hits, noise beds.
        // Indices must stay in step with the definition table in SampleBank.cpp.
        namespace smp
        {
            constexpr int chipCoin     = 10;
            constexpr int arcadeLaser  = 11;
            constexpr int chipPowerUp  = 12;
            constexpr int metalHit     = 13;
            constexpr int glassBreak   = 14;
            constexpr int clickTick    = 15;
            constexpr int tapeHiss     = 16;
            constexpr int vinylCrackle = 17;
            constexpr int radioStatic  = 18;
            constexpr int voiceAh      = 19;
            constexpr int retroEngine  = 20;

            constexpr int grandPiano   = 5;
        }
    }

    /**
        Patches built on the character samples rather than the instruments.

        The point of these is that the source is something no synthesiser can
        arrive at by adding partials together: a hard-quantised arcade blip, a
        struck sheet of metal, the noise floor of a tape. Played back at pitch
        and put through the same filter, envelopes and effects as everything
        else, that material stops being a sound effect and becomes an
        instrument with a character of its own.
    */
    std::vector<Preset> characterPresets()
    {
        std::vector<Preset> list;

        // -- arcade ---------------------------------------------------------

        // The coin blip is two fixed pitches, so transposing it moves both and
        // keeps the interval. Arpeggiated, that interval becomes the hook.
        list.push_back (Build ("Coin Arp", "Arp")
            .voices (10)
            .arp (0, 7, 2, 0.5f, 0.0f)
            .sampleOsc (0, smp::chipCoin, 0.8f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 5000.0f, 0.08f, 0.1f, 1.0f, 0.4f)
            .env (0, 0.5f, 900.0f, 0.0f, 160.0f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.28f, 0.66f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Coin Bell Keys", "Bell")
            .voices (10)
            .sampleOsc (0, smp::chipCoin, 0.55f)
            .osc (1, wt::FmBell, 0.36f, 0.30f)
            .tune (0, -1)
            .tune (1, 1)
            .filter (flt::LP24, 6000.0f, 0.06f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1800.0f, 0.0f, 400.0f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // The laser is a falling sweep, so every note lands as a downward
        // gesture rather than a pitch. Playing it low turns it into a drop.
        list.push_back (Build ("Laser Drop", "FX")
            .voices (4)
            .sampleOsc (0, smp::arcadeLaser, 0.9f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 3000.0f, 0.20f, 0.25f, 1.0f, 0.3f)
            .env (0, 0.5f, 2000.0f, 0.0f, 300.0f)
            .fx (FX::Distortion, 0.3f, 0.28f, 0.4f, 0.5f)
            .fx (FX::Reverb, 0.36f, 0.8f, 0.3f, 1.0f)
            .oversample (1)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Laser Zap Lead", "Lead")
            .voices (1, 1)
            .glide (30.0f, 2)
            .sampleOsc (0, smp::arcadeLaser, 0.6f)
            .osc (1, wt::BasicShapes, wt::square, 0.35f)
            .tune (0, 1)
            .filter (flt::LP24, 4000.0f, 0.18f, 0.2f, 1.0f, 0.6f)
            .env (0, 0.5f, 700.0f, 0.35f, 200.0f)
            .env (1, 0.5f, 120.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .fx (FX::BitCrusher, 0.22f, 0.42f, 0.35f, 0.4f)
            .fx (FX::Delay, 0.26f, 0.2f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        // A power-up is already an arpeggio. Pitched down and stretched it
        // becomes a riser, which is what that gesture is for.
        list.push_back (Build ("Power Up Riser", "FX")
            .voices (4)
            .sampleOsc (0, smp::chipPowerUp, 0.85f)
            .oscOff (1)
            .tune (0, -2)
            .filter (flt::LP24, 1200.0f, 0.25f, 0.15f, 1.0f, 0.3f)
            .env (0, 1.0f, 3000.0f, 0.0f, 500.0f)
            .env (1, 1.0f, 2600.0f, 0.0f, 400.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.55f)
            .fx (FX::Flanger, 0.3f, 0.12f, 0.45f, 0.55f)
            .fx (FX::Reverb, 0.45f, 0.86f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Arcade Sequence", "Sequence")
            .voices (10)
            .arp (5, 8, 2, 0.4f, 0.0f)
            .sampleOsc (0, smp::chipPowerUp, 0.8f)
            .oscOff (1)
            .filter (flt::BP12, 2400.0f, 0.30f, 0.15f, 1.0f, 0.4f)
            .env (0, 0.5f, 500.0f, 0.0f, 120.0f)
            .lfoSynced (0, 5, 6)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.30f, true)
            .fx (FX::BitCrusher, 0.2f, 0.4f, 0.32f, 0.4f)
            .fx (FX::Delay, 0.3f, 0.16f, 0.36f, 0.95f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        // -- struck things --------------------------------------------------

        list.push_back (Build ("Metal Bell Keys", "Bell")
            .voices (10)
            .sampleOsc (0, smp::metalHit, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 2400.0f, 0.0f, 500.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.22f, 0.24f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.4f, 0.82f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Anvil Pluck", "Pluck")
            .voices (10)
            .sampleOsc (0, smp::metalHit, 0.9f)
            .oscOff (1)
            .tune (0, 1)
            .filter (flt::LP24, 3400.0f, 0.16f, 0.15f, 1.0f, 0.55f)
            .env (0, 0.5f, 500.0f, 0.0f, 140.0f)
            .env (1, 0.5f, 140.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Delay, 0.24f, 0.2f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Glass Shard Pluck", "Pluck")
            .voices (10)
            .sampleOsc (0, smp::glassBreak, 0.7f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.28f)
            .tune (1, 1)
            .filter (flt::LP24, 5000.0f, 0.08f, 0.0f, 1.0f, 0.6f)
            .env (0, 0.5f, 600.0f, 0.0f, 200.0f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.32f, 0.9f)
            .fx (FX::Reverb, 0.38f, 0.8f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Glass Sparkle Bell", "Bell")
            .voices (10)
            .sampleOsc (0, smp::glassBreak, 0.45f)
            .osc (1, wt::FmBell, 0.40f, 0.40f)
            .tune (0, 1)
            .tune (1, 1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.7f)
            .env (0, 0.5f, 2000.0f, 0.0f, 600.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.76f, 0.6f)
            .fx (FX::Delay, 0.28f, 0.24f, 0.36f, 0.95f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        // A click has no pitch of its own, so it takes whatever the arp gives
        // it and turns into a rhythm part rather than a melody.
        list.push_back (Build ("Tick Sequence", "Sequence")
            .voices (8)
            .arp (4, 8, 1, 0.35f, 0.2f)
            .sampleOsc (0, smp::clickTick, 0.85f)
            .oscOff (1)
            .filter (flt::BP12, 2000.0f, 0.35f, 0.1f, 1.0f, 0.5f)
            .env (0, 0.5f, 200.0f, 0.0f, 80.0f)
            .fx (FX::Delay, 0.3f, 0.16f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.26f, 0.6f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        // -- noise beds -----------------------------------------------------
        //
        // These loop, so the sample stops being a hit and becomes a sustained
        // texture that the filter and envelopes shape like any other source.

        list.push_back (Build ("Tape Bed Pad", "Ambient")
            .voices (8)
            .sampleOsc (0, smp::tapeHiss, 0.30f, true)
            .osc (1, wt::Formant, 0.35f, 0.45f)
            .tune (1, 0)
            .unison (1, 5, 0.12f, 0.55f, 0.9f)
            .filter (flt::LP24, 2000.0f, 0.06f, 0.0f, 1.0f, 0.35f)
            .env (0, 800.0f, 3000.0f, 0.75f, 2200.0f)
            .lfoFree (0, 0, 0.09f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.16f, true)
            .fx (FX::Chorus, 0.3f, 0.08f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.55f, 0.92f, 0.2f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Vinyl Piano", "Keys")
            .voices (10)
            .sampleOsc (0, smp::grandPiano, 0.85f)
            // The crackle sits behind the piano and does not follow the note,
            // which is exactly what a record does.
            .noise (1, 0.03f, false)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 2.0f, 3600.0f, 0.0f, 450.0f)
            .velocity (0.7f)
            .fx (FX::Eq, 1.0f, 0.4f, 0.42f, 0.3f)
            .fx (FX::Reverb, 0.34f, 0.74f, 0.28f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Vinyl Crackle Bed", "Ambient")
            .voices (4)
            .sampleOsc (0, smp::vinylCrackle, 0.55f, true)
            .osc (1, wt::BasicShapes, wt::sine, 0.30f)
            .tune (1, -1)
            .filter (flt::LP24, 3000.0f, 0.05f, 0.0f, 1.0f, 0.3f)
            .env (0, 600.0f, 3000.0f, 0.8f, 2000.0f)
            .fx (FX::Eq, 1.0f, 0.45f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.5f, 0.9f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Broken Radio", "FX")
            .voices (4)
            .sampleOsc (0, smp::radioStatic, 0.7f, true)
            .oscOff (1)
            .filter (flt::BP12, 1400.0f, 0.45f, 0.2f, 1.0f, 0.4f)
            .env (0, 20.0f, 2000.0f, 0.7f, 500.0f)
            .lfoFree (0, 5, 3.2f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.45f, true)
            .route (Src::ModWheel, Dst::FilterCutoff, 0.4f)
            .fx (FX::BitCrusher, 0.3f, 0.45f, 0.4f, 0.5f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        // -- voice ----------------------------------------------------------

        list.push_back (Build ("Choir Ah Pad", "Pad")
            .voices (10)
            .sampleOsc (0, smp::voiceAh, 0.75f, true)
            .osc (1, wt::Formant, 0.28f, 0.25f)
            .tune (1, 0)
            .unison (1, 3, 0.10f, 0.5f, 0.8f)
            .filter (flt::LP24, 3000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 300.0f, 2600.0f, 0.85f, 1400.0f)
            .lfoFree (0, 0, 4.4f)
            .lfoShape (0, 1200.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Chorus, 0.3f, 0.1f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.5f, 0.9f, 0.22f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Choir Stab", "Chords")
            .voices (12)
            .sampleOsc (0, smp::voiceAh, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 3400.0f, 0.10f, 0.05f, 1.0f, 0.55f)
            .env (0, 6.0f, 600.0f, 0.0f, 320.0f)
            .env (1, 2.0f, 200.0f, 0.0f, 150.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .fx (FX::Delay, 0.22f, 0.28f, 0.3f, 0.8f)
            .fx (FX::Reverb, 0.4f, 0.82f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Choir Sparkle Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::voiceAh, 0.55f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.28f)
            .tune (1, 1)
            .unison (1, 5, 0.13f, 0.55f, 0.9f)
            .filter (flt::LP24, 2800.0f, 0.08f, 0.05f, 1.0f, 0.65f)
            .env (0, 18.0f, 1600.0f, 0.72f, 700.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.55f)
            .fx (FX::Delay, 0.20f, 0.32f, 0.3f, 0.75f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // -- engine ---------------------------------------------------------

        list.push_back (Build ("Engine Bass", "Bass")
            .voices (2, 1)
            .sampleOsc (0, smp::retroEngine, 0.7f, true)
            .oscOff (1)
            .tune (0, -1)
            .sub (0, 0.45f)
            .filter (flt::LP24, 500.0f, 0.20f, 0.3f, 1.0f, 0.2f)
            .env (0, 4.0f, 900.0f, 0.85f, 180.0f)
            .lfoSynced (0, 0, 6)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.25f)
            .fx (FX::Distortion, 0.35f, 0.32f, 0.45f, 0.5f)
            .oversample (1)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Engine Drone", "Ambient")
            .voices (4)
            .sampleOsc (0, smp::retroEngine, 0.6f, true)
            .osc (1, wt::BasicShapes, wt::sine, 0.35f)
            .tune (0, -1)
            .tune (1, -2)
            .filter (flt::LP24, 900.0f, 0.10f, 0.15f, 1.0f, 0.25f)
            .env (0, 900.0f, 3000.0f, 0.85f, 2400.0f)
            .lfoFree (0, 0, 0.07f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.20f, true)
            .fx (FX::Reverb, 0.55f, 0.92f, 0.22f, 1.0f)
            .master (-10.0f)
            .done());


        return list;
    }
}
