#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        namespace smp
        {
            constexpr int reedOrgan   = 42;
            constexpr int accordion   = 43;
            constexpr int pipeOrgan   = 44;
            constexpr int trumpet     = 45;
            constexpr int saxophone   = 46;
            constexpr int shakuhachi  = 47;
            constexpr int banjo       = 48;
            constexpr int koto        = 49;
            constexpr int dulcimer    = 50;
            constexpr int celesta     = 51;
            constexpr int tabla       = 52;
            constexpr int djembe      = 53;

            constexpr int grandPiano  = 5;
            constexpr int nylonGuitar = 0;
        }
    }

    /**
        Reeds, pipes, hand percussion and the strings that are neither guitar
        nor piano.

        Several of these lean on the second filter, which is what it is for: a
        low-pass and a high-pass in serial leave a band, which is how a horn or
        a telephone-narrow reed is shaped, while the same pair in parallel
        leaves everything except a band, which hollows a sound out without
        making it quiet.
    */
    std::vector<Preset> worldPresets()
    {
        std::vector<Preset> list;

        // -- reeds and pipes -------------------------------------------------

        list.push_back (Build ("Reed Organ", "Keys")
            .voices (12)
            .sampleOsc (0, smp::reedOrgan, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.35f)
            .env (0, 25.0f, 800.0f, 0.92f, 180.0f)
            .velocity (0.35f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.55f, 0.4f)
            .fx (FX::Reverb, 0.24f, 0.65f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Harmonium Pad", "Pad")
            .voices (12)
            .sampleOsc (0, smp::reedOrgan, 0.7f, true)
            .osc (1, wt::Formant, 0.30f, 0.22f)
            .tune (1, 0)
            .unison (1, 3, 0.09f, 0.5f, 0.8f, 0.2f)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.0f, 1.0f, 0.35f)
            .env (0, 400.0f, 2200.0f, 0.9f, 1400.0f)
            .lfoFree (0, 0, 0.11f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.14f, true)
            .fx (FX::Chorus, 0.26f, 0.09f, 0.36f, 0.3f)
            .fx (FX::Reverb, 0.46f, 0.88f, 0.22f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Accordion", "Keys")
            .voices (12)
            .sampleOsc (0, smp::accordion, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 30.0f, 700.0f, 0.90f, 160.0f)
            .velocity (0.4f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.6f, 0.42f)
            .fx (FX::Reverb, 0.26f, 0.68f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        // A reed through a band leaves the honk and nothing else, which is
        // what a bellows instrument heard across a room actually sounds like.
        list.push_back (Build ("Bandoneon Band", "Keys")
            .voices (10)
            .sampleOsc (0, smp::accordion, 0.9f, true)
            .oscOff (1)
            .filter (flt::LP24, 2200.0f, 0.10f, 0.05f, 1.0f, 0.35f)
            .filter2 (flt::HP24, 320.0f, 0.12f)
            .env (0, 40.0f, 800.0f, 0.88f, 200.0f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.30f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Pipe Organ", "Keys")
            .voices (14)
            .sampleOsc (0, smp::pipeOrgan, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.35f)
            // No velocity at all: an organ pipe is either sounding or not.
            .env (0, 18.0f, 500.0f, 1.0f, 140.0f)
            .velocity (0.0f)
            .fx (FX::Reverb, 0.42f, 0.90f, 0.24f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Church Organ Chords", "Chords")
            .voices (14)
            .sampleOsc (0, smp::pipeOrgan, 0.6f, true)
            .osc (1, wt::Organ, 0.35f, 0.28f)
            .tune (1, 1)
            .filter (flt::LP24, 4500.0f, 0.04f, 0.0f, 1.0f, 0.35f)
            .env (0, 30.0f, 600.0f, 1.0f, 320.0f)
            .velocity (0.0f)
            .fx (FX::Chorus, 0.18f, 0.08f, 0.30f, 0.22f)
            .fx (FX::Reverb, 0.52f, 0.93f, 0.20f, 1.0f)
            .master (-10.0f)
            .done());

        // -- horns -----------------------------------------------------------

        // Brass gets brighter the harder it is blown, and the curve is what
        // makes that read as effort rather than as a volume change: a linear
        // routing opens the filter evenly, this one holds back and then gives.
        list.push_back (Build ("Solo Trumpet", "Brass")
            .voices (1, 1)
            .glide (30.0f, 2)
            .sampleOsc (0, smp::trumpet, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 1400.0f, 0.10f, 0.15f, 1.0f, 0.5f)
            .env (0, 45.0f, 700.0f, 0.88f, 200.0f)
            .env (1, 70.0f, 600.0f, 0.55f, 220.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.45f, -0.6f)
            .routeCurved (Src::Velocity, Dst::FilterCutoff, 0.30f, -0.5f)
            .lfoFree (0, 0, 5.4f)
            .lfoShape (0, 700.0f, 0.2f)
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.018f, true)
            .fx (FX::Eq, 1.0f, 0.5f, 0.62f, 0.45f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Muted Trumpet", "Brass")
            .voices (1, 1)
            .glide (26.0f, 2)
            .sampleOsc (0, smp::trumpet, 0.9f, true)
            .oscOff (1)
            // A harmon mute is a band-pass: it takes the bottom out as much as
            // the top, which is why a muted trumpet cuts without being loud.
            .filter (flt::LP24, 3000.0f, 0.20f, 0.20f, 1.0f, 0.5f)
            .filter2 (flt::HP24, 700.0f, 0.25f)
            .env (0, 35.0f, 600.0f, 0.85f, 180.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.5f)
            .fx (FX::Delay, 0.16f, 0.26f, 0.26f, 0.7f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Saxophone", "Brass")
            .voices (1, 1)
            .glide (35.0f, 2)
            .sampleOsc (0, smp::saxophone, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 2400.0f, 0.10f, 0.12f, 1.0f, 0.45f)
            .env (0, 40.0f, 700.0f, 0.88f, 220.0f)
            .env (1, 60.0f, 600.0f, 0.5f, 240.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.35f, -0.5f)
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 900.0f, 0.2f)
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.020f, true)
            .fx (FX::Eq, 1.0f, 0.5f, 0.58f, 0.45f)
            .fx (FX::Reverb, 0.28f, 0.70f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Sax Section Stab", "Brass")
            .voices (8)
            .sampleOsc (0, smp::saxophone, 0.75f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.20f)
            .tune (1, 0, 0, 8.0f)
            .filter (flt::LP24, 2600.0f, 0.14f, 0.18f, 1.0f, 0.45f)
            .env (0, 14.0f, 380.0f, 0.0f, 200.0f)
            .env (1, 8.0f, 200.0f, 0.0f, 140.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.45f, -0.4f)
            .fx (FX::Distortion, 0.14f, 0.20f, 0.5f, 0.5f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Shakuhachi", "Keys")
            .voices (1, 1)
            .glide (40.0f, 2)
            .sampleOsc (0, smp::shakuhachi, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 3600.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 60.0f, 900.0f, 0.85f, 300.0f)
            .lfoFree (0, 0, 4.6f)
            .lfoShape (0, 1100.0f, 0.25f)
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.024f, true)
            .fx (FX::Delay, 0.22f, 0.28f, 0.28f, 0.85f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        // -- plucked ----------------------------------------------------------

        list.push_back (Build ("Banjo", "Plucked")
            .voices (10)
            .sampleOsc (0, smp::banjo, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1400.0f, 0.0f, 200.0f)
            .velocity (0.55f)
            .fx (FX::Eq, 1.0f, 0.52f, 0.66f, 0.45f)
            .fx (FX::Reverb, 0.20f, 0.55f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Banjo Roll", "Arp")
            .voices (12)
            .arp (0, 8, 2, 0.45f, 0.10f)
            .sampleOsc (0, smp::banjo, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 700.0f, 0.0f, 140.0f)
            .fx (FX::Delay, 0.18f, 0.18f, 0.26f, 0.7f)
            .fx (FX::Reverb, 0.26f, 0.64f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Koto", "Plucked")
            .voices (10)
            .sampleOsc (0, smp::koto, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 3000.0f, 0.0f, 400.0f)
            .velocity (0.55f)
            .bendRange (2)
            .fx (FX::Delay, 0.18f, 0.24f, 0.28f, 0.75f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Koto Seq", "Sequence")
            .voices (12)
            .arp (4, 7, 2, 0.5f, 0.18f)
            .sampleOsc (0, smp::koto, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 4200.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1000.0f, 0.0f, 220.0f)
            .fx (FX::Delay, 0.24f, 0.20f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Hammered Dulcimer", "Plucked")
            .voices (12)
            .sampleOsc (0, smp::dulcimer, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6500.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 2200.0f, 0.0f, 320.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.16f, 0.22f, 0.26f, 0.7f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Dulcimer Sparkle", "Bell")
            .voices (12)
            .sampleOsc (0, smp::dulcimer, 0.55f)
            .osc (1, wt::FmBell, 0.36f, 0.28f)
            .tune (1, 1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.6f)
            .env (0, 0.5f, 2000.0f, 0.0f, 500.0f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.55f)
            .fx (FX::Delay, 0.26f, 0.24f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Celesta", "Bell")
            .voices (12)
            .sampleOsc (0, smp::celesta, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 8000.0f, 0.04f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 2000.0f, 0.0f, 400.0f)
            .velocity (0.5f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Celesta Sparkle Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::celesta, 0.35f)
            .osc (1, wt::BasicShapes, wt::saw, 0.42f)
            .unison (1, 7, 0.15f, 0.55f, 0.9f, 0.12f)
            .filter (flt::LP24, 3200.0f, 0.09f, 0.06f, 1.0f, 0.8f)
            .env (0, 3.0f, 1100.0f, 0.70f, 440.0f)
            .env (1, 1.0f, 250.0f, 0.0f, 250.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.28f, 0.4f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.6f)
            .fx (FX::Delay, 0.26f, 0.28f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // -- hand percussion --------------------------------------------------

        list.push_back (Build ("Tabla", "Pluck")
            .voices (8)
            .sampleOsc (0, smp::tabla, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.05f, 1.0f, 0.4f)
            .env (0, 0.5f, 1000.0f, 0.0f, 150.0f)
            .velocity (0.65f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.55f, 0.4f)
            .fx (FX::Reverb, 0.18f, 0.5f, 0.32f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Tabla Seq", "Sequence")
            .voices (8)
            .arp (4, 8, 1, 0.4f, 0.22f)
            .sampleOsc (0, smp::tabla, 0.9f)
            .oscOff (1)
            .filter (flt::BP12, 900.0f, 0.20f, 0.05f, 1.0f, 0.4f)
            .env (0, 0.5f, 500.0f, 0.0f, 110.0f)
            .fx (FX::Delay, 0.20f, 0.18f, 0.28f, 0.7f)
            .fx (FX::Reverb, 0.24f, 0.60f, 0.30f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Djembe", "Pluck")
            .voices (8)
            .sampleOsc (0, smp::djembe, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.08f, 1.0f, 0.35f)
            .env (0, 0.5f, 800.0f, 0.0f, 130.0f)
            .velocity (0.7f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.5f, 0.4f)
            .fx (FX::Reverb, 0.16f, 0.48f, 0.32f, 1.0f)
            .master (-6.0f)
            .done());

        // Parallel rather than serial: the low-pass keeps the body and the
        // high-pass keeps the slap, and summing them leaves the mid-range
        // scooped out - which is what a drum sounds like close-miked.
        list.push_back (Build ("Djembe Scooped", "Pluck")
            .voices (8)
            .sampleOsc (0, smp::djembe, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 260.0f, 0.15f, 0.15f, 1.0f, 0.2f)
            .filter2 (flt::HP24, 2400.0f, 0.15f, true)
            .env (0, 0.5f, 800.0f, 0.0f, 130.0f)
            .velocity (0.7f)
            .fx (FX::Distortion, 0.12f, 0.18f, 0.5f, 0.5f)
            .fx (FX::Reverb, 0.20f, 0.55f, 0.32f, 1.0f)
            .master (-7.0f)
            .done());

        // -- hybrids ----------------------------------------------------------

        list.push_back (Build ("Organ Piano Layer", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.65f)
            .osc (1, wt::Organ, 0.30f, 0.26f)
            .tune (1, 0)
            .filter (flt::LP24, 4200.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 4.0f, 2600.0f, 0.25f, 420.0f)
            .fx (FX::Chorus, 0.18f, 0.10f, 0.30f, 0.24f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Folk Layer Pluck", "Plucked")
            .voices (12)
            .sampleOsc (0, smp::nylonGuitar, 0.60f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.18f)
            .tune (1, 1)
            .filter (flt::LP24, 4600.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .filter2 (flt::HP12, 180.0f, 0.05f)
            .env (0, 1.0f, 2400.0f, 0.0f, 300.0f)
            .velocity (0.55f)
            .fx (FX::Delay, 0.18f, 0.24f, 0.28f, 0.75f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        return list;
    }
}
