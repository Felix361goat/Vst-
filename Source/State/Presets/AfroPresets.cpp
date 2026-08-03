#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        namespace smp
        {
            constexpr int nylonGuitar   = 0;
            constexpr int steelGuitar   = 1;
            constexpr int mutedPluck    = 3;
            constexpr int fingerBass    = 4;
            constexpr int grandPiano    = 5;
            constexpr int electricPiano = 6;
            constexpr int kalimba       = 7;
            constexpr int steelDrum     = 8;
            constexpr int marimbaBar    = 9;
            constexpr int harp          = 21;
            constexpr int hangDrum      = 26;
            constexpr int panFlute      = 29;
            constexpr int choirOo       = 30;
        }
    }

    /**
        Afroswing, afrobeats and the melodic UK sound built on top of them.

        These lean on the modelled instruments rather than the wavetables,
        because the genre does: the parts are played on plucked strings, thumb
        pianos and tuned metal, and the thing that identifies them is the
        attack of a real instrument sitting in a lot of space. A saw through a
        filter cannot get there however it is shaped.

        Almost everything here is short and dry-ish in the low end and wet
        above it, which is how these parts sit around a vocal without
        competing with it.
    */
    std::vector<Preset> afroPresets()
    {
        std::vector<Preset> list;

        // -- guitar parts ---------------------------------------------------

        // The two-note figure that opens half the genre: muted, tight, and
        // arpeggiated so it plays itself once a chord is held.
        list.push_back (Build ("Afroswing Riff", "Plucked")
            .voices (10)
            .arp (0, 7, 1, 0.42f, 0.20f)
            .sampleOsc (0, smp::mutedPluck, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 3400.0f, 0.10f, 0.08f, 1.0f, 0.5f)
            .env (0, 0.5f, 600.0f, 0.0f, 130.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.26f, 0.6f)
            .fx (FX::Reverb, 0.20f, 0.55f, 0.30f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Afroswing Riff Wide", "Plucked")
            .voices (10)
            .arp (2, 7, 2, 0.45f, 0.22f)
            .sampleOsc (0, smp::mutedPluck, 0.7f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.16f)
            .tune (1, 1)
            .filter (flt::LP24, 4000.0f, 0.09f, 0.06f, 1.0f, 0.5f)
            .env (0, 0.5f, 700.0f, 0.0f, 160.0f)
            .fx (FX::Chorus, 0.26f, 0.18f, 0.38f, 0.32f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.32f, 0.8f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Nylon Fingerpick", "Plucked")
            .voices (12)
            .arp (4, 7, 1, 0.6f, 0.16f)
            .sampleOsc (0, smp::nylonGuitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5500.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1800.0f, 0.0f, 300.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.18f, 0.22f, 0.26f, 0.7f)
            .fx (FX::Reverb, 0.26f, 0.65f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Highlife Guitar", "Plucked")
            .voices (12)
            .arp (0, 8, 2, 0.4f, 0.24f)
            .sampleOsc (0, smp::steelGuitar, 0.8f)
            .oscOff (1)
            // The bandpass is what a small amplifier through a small speaker
            // does, and the genre grew up on exactly that.
            .filter (flt::BP12, 1800.0f, 0.18f, 0.1f, 1.0f, 0.55f)
            .env (0, 0.5f, 900.0f, 0.0f, 180.0f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        // -- thumb piano and tuned metal ------------------------------------

        list.push_back (Build ("Afro Kalimba Riff", "Sequence")
            .voices (12)
            .arp (0, 7, 2, 0.45f, 0.22f)
            .sampleOsc (0, smp::kalimba, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6500.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1000.0f, 0.0f, 200.0f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.32f, 0.72f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Kalimba Marimba Stack", "Bell")
            .voices (12)
            .sampleOsc (0, smp::kalimba, 0.6f)
            .osc (1, wt::FmBell, 0.28f, 0.22f)
            .tune (1, 1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1600.0f, 0.0f, 320.0f)
            .velocity (0.55f)
            .fx (FX::Delay, 0.24f, 0.24f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Hang Melody", "Bell")
            .voices (10)
            .sampleOsc (0, smp::hangDrum, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4600.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 2400.0f, 0.0f, 380.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.22f, 0.24f, 0.28f, 0.8f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.28f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Pan Melody", "Sequence")
            .voices (10)
            .arp (2, 7, 2, 0.5f, 0.24f)
            .sampleOsc (0, smp::steelDrum, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5200.0f, 0.06f, 0.05f, 1.0f, 0.45f)
            .env (0, 0.5f, 900.0f, 0.0f, 200.0f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Marimba Afro Seq", "Sequence")
            .voices (12)
            .arp (3, 7, 2, 0.45f, 0.20f)
            .sampleOsc (0, smp::marimbaBar, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 900.0f, 0.0f, 180.0f)
            .fx (FX::Delay, 0.24f, 0.20f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.30f, 0.70f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        // -- keys -----------------------------------------------------------

        list.push_back (Build ("Afro Piano Riff", "Sequence")
            .voices (12)
            .arp (4, 6, 1, 0.55f, 0.18f)
            .sampleOsc (0, smp::grandPiano, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5200.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 2200.0f, 0.0f, 300.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.20f, 0.24f, 0.28f, 0.75f)
            .fx (FX::Reverb, 0.30f, 0.70f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Amapiano Keys", "Keys")
            .voices (12)
            .sampleOsc (0, smp::electricPiano, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 4200.0f, 0.06f, 0.06f, 1.0f, 0.4f)
            .env (0, 2.0f, 2400.0f, 0.0f, 320.0f)
            .velocity (0.6f)
            // The slow pan wobble is the log-drum era's Rhodes sound.
            .lfoSynced (0, 0, 5)
            .route (Src::Lfo1, Dst::Osc1Pan, 0.35f, true)
            .fx (FX::Chorus, 0.26f, 0.12f, 0.36f, 0.3f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Afro Chord Stab", "Chords")
            .voices (12)
            .sampleOsc (0, smp::electricPiano, 0.6f)
            .osc (1, wt::BasicShapes, wt::saw, 0.24f)
            .tune (1, 0)
            .unison (1, 5, 0.12f, 0.55f, 0.9f, 0.12f)
            .filter (flt::LP24, 2200.0f, 0.12f, 0.06f, 1.0f, 0.6f)
            .env (0, 2.0f, 700.0f, 0.0f, 320.0f)
            .env (1, 1.0f, 220.0f, 0.0f, 140.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.30f, 0.8f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        // -- bass -----------------------------------------------------------

        list.push_back (Build ("Afro Log Bass", "Bass")
            .voices (1, 1)
            .glide (55.0f, 2)
            .osc (0, wt::BasicShapes, wt::sine, 0.95f)
            .oscOff (1)
            .tune (0, -1)
            // The pitch fall is the log drum: a bass note that arrives from
            // above rather than simply starting.
            .filter (flt::LP24, 420.0f, 0.06f, 0.10f)
            .env (0, 2.0f, 900.0f, 0.55f, 160.0f)
            .env (1, 1.0f, 140.0f, 0.0f, 90.0f)
            .route (Src::Env2, Dst::Osc1Pitch, 0.09f)
            .fx (FX::Eq, 1.0f, 0.6f, 0.4f, 0.3f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Afro Picked Bass", "Bass")
            .voices (1, 1)
            .glide (28.0f, 2)
            .sampleOsc (0, smp::fingerBass, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 1100.0f, 0.06f, 0.12f, 1.0f, 0.28f)
            .env (0, 1.0f, 2000.0f, 0.0f, 160.0f)
            .velocity (0.55f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.45f, 0.35f)
            .master (-6.0f)
            .done());

        // -- tops and pads ---------------------------------------------------

        list.push_back (Build ("Afro Flute Top", "Lead")
            .voices (1, 1)
            .glide (26.0f, 2)
            .sampleOsc (0, smp::panFlute, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 4200.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 30.0f, 700.0f, 0.85f, 220.0f)
            .lfoFree (0, 0, 5.2f)
            .lfoShape (0, 800.0f, 0.2f)
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.020f, true)
            .fx (FX::Delay, 0.24f, 0.24f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Afro Harp Roll", "Arp")
            .voices (12)
            .arp (2, 8, 3, 0.65f, 0.0f)
            .sampleOsc (0, smp::harp, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1600.0f, 0.0f, 320.0f)
            .fx (FX::Delay, 0.22f, 0.20f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Afro Choir Bed", "Pad")
            .voices (10)
            .sampleOsc (0, smp::choirOo, 0.75f, true)
            .osc (1, wt::Formant, 0.30f, 0.22f)
            .tune (1, 0)
            .unison (1, 3, 0.10f, 0.5f, 0.8f, 0.2f)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 350.0f, 2400.0f, 0.85f, 1500.0f)
            .fx (FX::Chorus, 0.28f, 0.09f, 0.38f, 0.3f)
            .fx (FX::Reverb, 0.46f, 0.88f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        // -- UK drill and melodic rap ---------------------------------------

        // Drill leans on plucked material played high and dry, so the slide in
        // the bass has room underneath it.
        list.push_back (Build ("Drill Guitar Loop", "Drill")
            .voices (10)
            .arp (0, 7, 2, 0.4f, 0.0f)
            .sampleOsc (0, smp::nylonGuitar, 0.85f)
            .oscOff (1)
            .tune (0, 1)
            .filter (flt::LP24, 4200.0f, 0.08f, 0.05f, 1.0f, 0.5f)
            .env (0, 0.5f, 800.0f, 0.0f, 180.0f)
            .fx (FX::Delay, 0.20f, 0.18f, 0.26f, 0.7f)
            .fx (FX::Reverb, 0.24f, 0.62f, 0.30f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Drill Bell Loop", "Drill")
            .voices (10)
            .arp (5, 7, 2, 0.4f, 0.0f)
            .sampleOsc (0, smp::marimbaBar, 0.5f)
            .osc (1, wt::FmBell, 0.44f, 0.35f)
            .tune (0, 1)
            .tune (1, 1)
            .filter (flt::LP24, 5000.0f, 0.06f, 0.0f, 1.0f, 0.6f)
            .env (0, 0.5f, 900.0f, 0.0f, 260.0f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Melodic Rap Keys", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.8f)
            .oscOff (1)
            // Rolled off hard and drenched: the lo-fi piano loop.
            .filter (flt::LP24, 1800.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 3.0f, 3200.0f, 0.0f, 500.0f)
            .velocity (0.7f)
            .fx (FX::Eq, 1.0f, 0.40f, 0.38f, 0.3f)
            .fx (FX::Delay, 0.18f, 0.28f, 0.26f, 0.75f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.26f, 1.0f)
            .master (-6.0f)
            .done());

        return list;
    }
}
