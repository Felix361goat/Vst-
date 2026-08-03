#include "State/PresetBuilder.h"

namespace nog::presets
{
    // Future bass is built on chords that move while they are held: the pitch
    // wobbles, the wavetable position slides, and the filter breathes. Playing
    // one static chord should already sound like an arrangement.
    std::vector<Preset> futureBassPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Future Chord Wide", "Future Bass")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::BasicShapes, wt::saw, 0.45f)
            .tune (1, 0, 0, 11.0f)
            .unison (0, 7, 0.20f, 0.62f, 1.0f)
            .unison (1, 5, 0.16f, 0.58f, 0.85f)
            .filter (flt::LP24, 2200.0f, 0.14f, 0.1f, 1.0f, 0.55f)
            .env (0, 25.0f, 900.0f, 0.85f, 500.0f)
            // The slow pitch drift is what makes a held chord feel alive.
            .lfoFree (0, 0, 0.9f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.012f, true)
            .route (Src::Lfo1, Dst::Osc2Pitch, -0.010f, true)
            .lfoSynced (1, 0, 4)
            .route (Src::Lfo2, Dst::FilterCutoff, 0.18f, true)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.55f)
            .fx (FX::Delay, 0.22f, 0.30f, 0.35f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.82f, 0.26f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Future Chord Warm", "Future Bass")
            .voices (16)
            .osc (0, wt::HarmonicSweep, 0.55f, 0.6f)
            .osc (1, wt::BasicShapes, 0.42f, 0.45f)
            .tune (1, -1)
            .unison (0, 5, 0.16f, 0.55f, 0.9f)
            .filter (flt::LP24, 1500.0f, 0.12f, 0.08f, 1.0f, 0.45f)
            .env (0, 40.0f, 1000.0f, 0.85f, 600.0f)
            .lfoFree (0, 0, 0.7f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.16f, true)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.009f, true)
            .fx (FX::Chorus, 0.26f, 0.15f, 0.4f, 0.2f)
            .fx (FX::Reverb, 0.38f, 0.85f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Future Growl Chord", "Future Bass")
            .voices (12)
            .osc (0, wt::Growl, 0.35f, 0.62f)
            .osc (1, wt::BasicShapes, wt::saw, 0.42f)
            .unison (0, 3, 0.12f, 0.5f, 0.8f)
            .unison (1, 5, 0.16f, 0.55f, 0.9f)
            .filter (flt::LP24, 1600.0f, 0.22f, 0.25f, 1.0f, 0.4f)
            .env (0, 15.0f, 800.0f, 0.85f, 400.0f)
            .lfoSynced (0, 1, 6)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.45f)
            .route (Src::ModWheel, Dst::Osc1WtPos, 0.35f)
            .fx (FX::Distortion, 0.28f, 0.28f, 0.5f, 0.5f)
            .fx (FX::Reverb, 0.28f, 0.72f, 0.3f, 1.0f)
            .oversample (1)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Future Vocal Chord", "Future Bass")
            .voices (16)
            .osc (0, wt::Formant, 0.28f, 0.6f)
            .osc (1, wt::BasicShapes, wt::saw, 0.42f)
            .unison (0, 3, 0.11f, 0.5f, 0.85f)
            .unison (1, 5, 0.15f, 0.55f, 0.95f)
            .filter (flt::LP24, 1900.0f, 0.15f, 0.1f, 1.0f, 0.5f)
            .env (0, 30.0f, 900.0f, 0.85f, 500.0f)
            .lfoFree (0, 0, 0.5f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.28f, true)
            .lfoFree (1, 0, 1.1f)
            .route (Src::Lfo2, Dst::Osc1Pitch, 0.010f, true)
            .fx (FX::Delay, 0.22f, 0.28f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.40f, 0.85f, 0.26f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Future Bass Growl", "Future Bass")
            .voices (4, 1)
            .osc (0, wt::Growl, 0.3f, 0.7f)
            .osc (1, wt::BasicShapes, wt::saw, 0.35f)
            .tune (0, -1)
            .tune (1, -1)
            .sub (0, 0.45f)
            .filter (flt::LP24, 900.0f, 0.35f, 0.35f)
            .env (0, 5.0f, 700.0f, 0.9f, 180.0f)
            .lfoSynced (0, 1, 6)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.55f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.25f)
            .fx (FX::Distortion, 0.45f, 0.4f, 0.45f, 0.5f)
            .oversample (2)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Future Pluck Stack", "Future Bass")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.6f)
            .osc (1, wt::FmBell, 0.32f, 0.3f)
            .tune (1, 1)
            .unison (0, 7, 0.18f, 0.6f, 0.95f)
            .filter (flt::LP24, 1400.0f, 0.2f, 0.1f, 1.0f, 0.6f)
            .env (0, 1.0f, 420.0f, 0.0f, 350.0f, 0.0f, 0.0f, 0.4f)
            .env (1, 0.5f, 200.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.4f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Eq, 1.0f, 0.45f, 0.7f, 0.55f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.38f, 0.95f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Melodic Dubstep Lead", "Future Bass")
            .voices (10)
            .osc (0, wt::BasicShapes, wt::saw, 0.65f)
            .osc (1, wt::HardSync, 0.4f, 0.35f)
            .unison (0, 9, 0.22f, 0.65f, 1.0f)
            .filter (flt::LP24, 2600.0f, 0.18f, 0.2f, 1.0f, 0.6f)
            .env (0, 8.0f, 900.0f, 0.85f, 400.0f)
            .env (1, 3.0f, 300.0f, 0.3f, 300.0f)
            .route (Src::Env2, Dst::Osc2WtPos, 0.4f)
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 600.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.008f, true)
            .fx (FX::Distortion, 0.25f, 0.25f, 0.55f, 0.5f)
            .fx (FX::Delay, 0.24f, 0.28f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.78f, 0.28f, 1.0f)
            .oversample (1)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Wobble Chord", "Future Bass")
            .voices (12)
            .osc (0, wt::BasicShapes, wt::saw, 0.65f)
            .oscOff (1)
            .unison (0, 5, 0.18f, 0.6f, 0.95f)
            .filter (flt::LP24, 800.0f, 0.35f, 0.15f)
            .env (0, 10.0f, 800.0f, 0.9f, 300.0f)
            .lfoSynced (0, 0, 6)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.4f)
            .fx (FX::Delay, 0.2f, 0.26f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.3f, 0.75f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        return list;
    }

    std::vector<Preset> trapPresets()
    {
        std::vector<Preset> list;

        list.push_back (Build ("Trap Bell", "Trap")
            .voices (12)
            .osc (0, wt::FmBell, 0.45f, 0.7f)
            .osc (1, wt::BasicShapes, wt::sine, 0.3f)
            .tune (0, 1)
            .filter (flt::LP24, 4200.0f, 0.06f, 0.0f, 1.0f, 0.8f)
            .env (0, 0.5f, 900.0f, 0.0f, 620.0f, 0.0f, 0.0f, 0.55f)
            .env (1, 0.5f, 200.0f, 0.0f, 180.0f)
            .route (Src::Env2, Dst::Osc1WtPos, 0.26f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.72f, 0.6f)
            .fx (FX::Delay, 0.24f, 0.22f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.8f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Dark Trap Bell", "Trap")
            .voices (12)
            .osc (0, wt::FmBell, 0.62f, 0.68f)
            .oscOff (1)
            .filter (flt::LP24, 1800.0f, 0.12f, 0.1f, 1.0f, 0.6f)
            .env (0, 0.5f, 1200.0f, 0.0f, 800.0f, 0.0f, 0.0f, 0.55f)
            .lfoFree (0, 0, 0.3f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.12f, true)
            .fx (FX::Delay, 0.26f, 0.26f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.42f, 0.88f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Trap Flute", "Trap")
            .voices (1, 1)
            .glide (38.0f, 2)
            // A flute is very close to a sine with a quiet octave above it.
            // Anything more than that stops sounding like a flute.
            .osc (0, wt::BasicShapes, 0.06f, 0.8f)
            .osc (1, wt::BasicShapes, wt::sine, 0.16f)
            .tune (1, 1)
            // Breath is a chiff on the attack, not a layer. It used to sustain
            // underneath the note, which read as hiss rather than as playing.
            .noise (1, 0.05f)
            .filter (flt::LP24, 3000.0f, 0.06f, 0.0f, 1.0f, 0.55f)
            .env (0, 55.0f, 700.0f, 0.85f, 220.0f)
            .env (1, 8.0f, 130.0f, 0.0f, 80.0f)
            .route (Src::Env2, Dst::NoiseLevel, 0.35f)
            .route (Src::Env2, Dst::FilterCutoff, 0.12f)
            // The rise time delays the vibrato, so a short note has none and a
            // held one swells - which is how it is actually played.
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 900.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.006f, true)
            .fx (FX::Delay, 0.20f, 0.26f, 0.28f, 0.8f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Trap Pluck Dark", "Trap")
            .voices (12)
            .osc (0, wt::Bass, 0.4f, 0.75f)
            .osc (1, wt::FmBell, 0.4f, 0.22f)
            .tune (1, 1)
            .filter (flt::LP24, 700.0f, 0.28f, 0.12f, 1.0f, 0.5f)
            .env (0, 1.0f, 400.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 170.0f, 0.0f, 150.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Delay, 0.24f, 0.24f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.75f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("808 Slide", "Trap")
            .voices (1, 1)
            .glide (85.0f, 2)
            .osc (0, wt::BasicShapes, wt::sine, 1.0f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 650.0f, 0.05f, 0.22f)
            .env (0, 2.0f, 2200.0f, 0.0f, 300.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 1.0f, 200.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::Osc1Pitch, 0.11f)
            .fx (FX::Distortion, 0.4f, 0.34f, 0.45f, 0.5f)
            .oversample (1)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Trap Choir", "Trap")
            .voices (16)
            .osc (0, wt::Formant, 0.22f, 0.7f)
            .oscOff (1)
            .unison (0, 5, 0.14f, 0.55f, 0.95f)
            .filter (flt::LP24, 1500.0f, 0.12f, 0.0f, 1.0f, 0.4f)
            .env (0, 300.0f, 1200.0f, 0.85f, 900.0f)
            .lfoFree (0, 0, 4.6f)
            .lfoShape (0, 1200.0f, 0.3f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.006f, true)
            .fx (FX::Reverb, 0.48f, 0.92f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Trap Lead Bright", "Trap")
            .voices (8, 1)
            .glide (30.0f, 2)
            .osc (0, wt::BasicShapes, wt::saw, 0.7f)
            .osc (1, wt::HarmonicSweep, 0.8f, 0.28f)
            .tune (1, 1)
            .unison (0, 5, 0.14f, 0.55f, 0.8f)
            .filter (flt::LP24, 2600.0f, 0.15f, 0.15f, 1.0f, 0.7f)
            .env (0, 4.0f, 700.0f, 0.8f, 260.0f)
            .fx (FX::Eq, 1.0f, 0.45f, 0.7f, 0.55f)
            .fx (FX::Delay, 0.24f, 0.24f, 0.35f, 0.9f)
            .fx (FX::Reverb, 0.3f, 0.72f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Trap Marimba", "Trap")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.18f, 0.82f)
            .osc (1, wt::FmBell, 0.28f, 0.2f)
            .tune (1, 1, 7)
            .filter (flt::LP24, 2000.0f, 0.1f, 0.0f, 1.0f, 0.6f)
            .env (0, 0.5f, 480.0f, 0.0f, 340.0f, 0.0f, 0.0f, 0.55f)
            .fx (FX::Delay, 0.2f, 0.2f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        return list;
    }

    std::vector<Preset> drillPresets()
    {
        std::vector<Preset> list;

        // Drill lives on sliding basses and cold, narrow melodic sounds. The
        // glide is always on, and always slower than a trap slide.
        list.push_back (Build ("Drill Slide Bass", "Drill")
            .voices (1, 1)
            .glide (120.0f, 1)
            .osc (0, wt::BasicShapes, wt::sine, 1.0f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 560.0f, 0.05f, 0.25f)
            .env (0, 3.0f, 2000.0f, 0.05f, 320.0f, 0.0f, 0.0f, 0.5f)
            .fx (FX::Distortion, 0.42f, 0.36f, 0.42f, 0.5f)
            .oversample (1)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Drill Dark Bell", "Drill")
            .voices (10)
            .osc (0, wt::FmBell, 0.58f, 0.68f)
            .oscOff (1)
            .filter (flt::LP24, 1500.0f, 0.15f, 0.1f, 1.0f, 0.55f)
            .env (0, 0.5f, 1000.0f, 0.0f, 700.0f, 0.0f, 0.0f, 0.55f)
            .fx (FX::Delay, 0.28f, 0.26f, 0.38f, 0.95f)
            .fx (FX::Reverb, 0.4f, 0.86f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Drill Cold Pluck", "Drill")
            .voices (10)
            .osc (0, wt::OddEven, 0.4f, 0.72f)
            .oscOff (1)
            .filter (flt::BP12, 1200.0f, 0.32f, 0.1f, 1.0f, 0.5f)
            .env (0, 0.5f, 300.0f, 0.0f, 220.0f, 0.0f, 0.0f, 0.5f)
            .env (1, 0.5f, 130.0f, 0.0f, 110.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.4f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.34f, 0.8f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Drill Strings", "Drill")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.6f)
            .osc (1, wt::OddEven, 0.6f, 0.32f)
            .unison (0, 5, 0.10f, 0.5f, 0.9f)
            .filter (flt::LP24, 1300.0f, 0.1f, 0.05f, 1.0f, 0.4f)
            .env (0, 260.0f, 1200.0f, 0.85f, 700.0f)
            .lfoFree (0, 0, 4.8f)
            .lfoShape (0, 1000.0f, 0.3f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Reverb, 0.42f, 0.88f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Drill Flute", "Drill")
            .voices (6, 1)
            .glide (55.0f, 2)
            .osc (0, wt::BasicShapes, 0.10f, 0.8f)
            .oscOff (1)
            .noise (1, 0.14f)
            .filter (flt::LP24, 2200.0f, 0.08f, 0.0f, 1.0f, 0.5f)
            .env (0, 55.0f, 700.0f, 0.75f, 300.0f)
            .lfoFree (0, 0, 4.9f)
            .lfoShape (0, 800.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.011f, true)
            .fx (FX::Delay, 0.24f, 0.26f, 0.32f, 0.9f)
            .fx (FX::Reverb, 0.38f, 0.85f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Drill Eerie Pad", "Drill")
            .voices (12)
            .osc (0, wt::Formant, 0.55f, 0.6f)
            .osc (1, wt::BasicShapes, 0.35f, 0.35f)
            .tune (1, -1)
            .unison (0, 3, 0.12f, 0.5f, 0.95f)
            .filter (flt::LP24, 900.0f, 0.2f, 0.05f)
            .env (0, 700.0f, 1600.0f, 0.85f, 1500.0f)
            .lfoFree (0, 6, 0.08f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.3f, true)
            .fx (FX::Reverb, 0.5f, 0.94f, 0.28f, 1.0f)
            .master (-11.0f)
            .done());

        return list;
    }

    std::vector<Preset> chordPresets()
    {
        std::vector<Preset> list;

        // Voiced for holding four or five notes at once: narrow unison, low
        // resonance and plenty of headroom, so a big chord does not turn to mud.
        list.push_back (Build ("Soft Chord Keys", "Chords")
            .voices (16)
            .osc (0, wt::BasicShapes, 0.32f, 0.6f)
            .osc (1, wt::BasicShapes, wt::sine, 0.35f)
            .tune (1, 1)
            .filter (flt::LP24, 1700.0f, 0.06f, 0.05f, 1.0f, 0.45f)
            .env (0, 8.0f, 1200.0f, 0.5f, 500.0f)
            .fx (FX::Chorus, 0.22f, 0.18f, 0.35f, 0.15f)
            .fx (FX::Reverb, 0.32f, 0.75f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Wide Chord Saw", "Chords")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.55f)
            .osc (1, wt::BasicShapes, 0.45f, 0.4f)
            .unison (0, 3, 0.10f, 0.5f, 1.0f)
            .filter (flt::LP24, 1800.0f, 0.1f, 0.08f, 1.0f, 0.45f)
            .env (0, 20.0f, 900.0f, 0.8f, 420.0f)
            .fx (FX::Eq, 1.0f, 0.48f, 0.62f, 0.5f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.3f, 1.0f)
            .master (-12.0f)
            .done());

        list.push_back (Build ("Organ Chord", "Chords")
            .voices (16)
            .osc (0, wt::Organ, 0.5f, 0.8f)
            .oscOff (1)
            .filter (flt::LP12, 5000.0f, 0.04f)
            .env (0, 5.0f, 300.0f, 1.0f, 100.0f)
            .fx (FX::Chorus, 0.3f, 0.5f, 0.5f, 0.2f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.35f, 0.95f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Bell Chord", "Chords")
            .voices (16)
            .osc (0, wt::FmBell, 0.35f, 0.55f)
            .osc (1, wt::BasicShapes, wt::sine, 0.4f)
            .filter (flt::LP24, 3200.0f, 0.05f, 0.0f, 1.0f, 0.7f)
            .env (0, 2.0f, 1600.0f, 0.25f, 900.0f, 0.0f, 0.0f, 0.5f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.32f, 0.9f)
            .fx (FX::Reverb, 0.4f, 0.85f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Lo-Fi Chord", "Chords")
            .voices (12)
            .osc (0, wt::BasicShapes, 0.3f, 0.7f)
            .oscOff (1)
            .filter (flt::LP24, 950.0f, 0.08f, 0.15f, 1.0f, 0.35f)
            .env (0, 12.0f, 1400.0f, 0.35f, 600.0f)
            .lfoFree (0, 6, 0.5f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::BitCrusher, 0.22f, 0.2f, 0.28f, 0.5f)
            .fx (FX::Eq, 1.0f, 0.58f, 0.34f, 0.4f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.42f, 0.9f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Stab Chord Bright", "Chords")
            .voices (16)
            .osc (0, wt::BasicShapes, wt::saw, 0.65f)
            .osc (1, wt::Organ, 0.55f, 0.3f)
            .unison (0, 3, 0.12f, 0.5f, 0.85f)
            .filter (flt::LP24, 1300.0f, 0.25f, 0.12f, 1.0f, 0.55f)
            .env (0, 1.0f, 380.0f, 0.0f, 260.0f, 0.0f, 0.0f, 0.45f)
            .env (1, 0.5f, 180.0f, 0.0f, 150.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Delay, 0.22f, 0.2f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.3f, 0.72f, 0.3f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Glass Chord", "Chords")
            .voices (16)
            .osc (0, wt::FmBell, 0.26f, 0.5f)
            .osc (1, wt::HarmonicSweep, 0.5f, 0.35f)
            .tune (1, 1)
            .filter (flt::LP24, 3600.0f, 0.06f, 0.0f, 1.0f, 0.65f)
            .env (0, 60.0f, 1500.0f, 0.55f, 900.0f)
            .lfoFree (0, 0, 0.35f)
            .route (Src::Lfo1, Dst::Osc1WtPos, 0.16f, true)
            .fx (FX::Delay, 0.24f, 0.32f, 0.34f, 0.95f)
            .fx (FX::Reverb, 0.44f, 0.9f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Dark Chord Pad", "Chords")
            .voices (16)
            .osc (0, wt::Bass, 0.4f, 0.6f)
            .osc (1, wt::BasicShapes, 0.4f, 0.35f)
            .tune (1, -1)
            .filter (flt::LP24, 800.0f, 0.12f, 0.1f, 1.0f, 0.35f)
            .env (0, 200.0f, 1400.0f, 0.75f, 900.0f)
            .fx (FX::Reverb, 0.42f, 0.9f, 0.32f, 1.0f)
            .master (-10.0f)
            .done());

        return list;
    }
}
