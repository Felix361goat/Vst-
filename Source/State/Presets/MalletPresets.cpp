#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        namespace smp
        {
            constexpr int vibraphone   = 57;
            constexpr int xylophone    = 58;
            constexpr int tubularBell  = 59;
            constexpr int crotale      = 60;
            constexpr int gamelanGong  = 61;
            constexpr int singingBowl  = 62;
            constexpr int steelTongue  = 63;
            constexpr int woodBlock    = 64;
            constexpr int mandolin     = 65;
            constexpr int oud          = 66;
            constexpr int guzheng      = 67;
            constexpr int choirEe      = 68;
            constexpr int whistle      = 69;
            constexpr int bottleBlow   = 70;
            constexpr int waterDrop    = 71;
            constexpr int iceCrackle   = 72;
            constexpr int reverseSwell = 73;
            constexpr int subDrop      = 74;

            constexpr int kalimba      = 7;
            constexpr int glockenspiel = 25;
            constexpr int celesta      = 51;
        }
    }

    /**
        Mallets, tuned metal and the sequenced parts built out of them.

        Most of these are arpeggiated, because that is what this material is
        for: a struck bar has an attack and a decay and nothing in between, so
        a held chord does almost nothing with it while a pattern does
        everything. Every one of them plays itself once a chord is held down.

        The rest are the individual sounds - a water drop, ice, a reversed
        swell - that are worth having precisely because no amount of filtering
        gets a wavetable anywhere near them.
    */
    std::vector<Preset> malletPresets()
    {
        std::vector<Preset> list;

        // -- vibraphone --------------------------------------------------------

        // The motor tremolo is baked into the sample, so this is the sound with
        // the vibrato already in it rather than an LFO pretending.
        list.push_back (Build ("Vibraphone", "Bell")
            .voices (12)
            .sampleOsc (0, smp::vibraphone, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 3000.0f, 0.0f, 500.0f)
            .velocity (0.55f)
            .fx (FX::Reverb, 0.30f, 0.74f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Vibraphone Seq", "Sequence")
            .voices (12)
            .arp (2, 7, 2, 0.55f, 0.16f)
            .sampleOsc (0, smp::vibraphone, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1200.0f, 0.0f, 260.0f)
            .fx (FX::Delay, 0.24f, 0.20f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Jazz Vibe Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::vibraphone, 0.75f)
            .osc (1, wt::BasicShapes, wt::sine, 0.18f)
            .tune (1, -1)
            .filter (flt::LP24, 3600.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 3200.0f, 0.15f, 700.0f)
            .fx (FX::Eq, 1.0f, 0.48f, 0.55f, 0.42f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        // -- xylophone and wood -------------------------------------------------

        list.push_back (Build ("Xylophone Seq", "Sequence")
            .voices (12)
            .arp (0, 8, 2, 0.35f, 0.14f)
            .sampleOsc (0, smp::xylophone, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 400.0f, 0.0f, 120.0f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.32f, 0.88f)
            .fx (FX::Reverb, 0.28f, 0.70f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Wood Block Seq", "Sequence")
            .voices (10)
            .arp (4, 8, 1, 0.35f, 0.22f)
            .sampleOsc (0, smp::woodBlock, 0.9f)
            .oscOff (1)
            .filter (flt::BP12, 1400.0f, 0.20f, 0.05f, 1.0f, 0.5f)
            .env (0, 0.5f, 260.0f, 0.0f, 80.0f)
            .fx (FX::Delay, 0.22f, 0.18f, 0.28f, 0.75f)
            .fx (FX::Reverb, 0.22f, 0.58f, 0.30f, 1.0f)
            .master (-8.0f)
            .done());

        // -- tuned metal --------------------------------------------------------

        list.push_back (Build ("Tubular Bells", "Bell")
            .voices (10)
            .sampleOsc (0, smp::tubularBell, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 4000.0f, 0.0f, 900.0f)
            .velocity (0.5f)
            .fx (FX::Reverb, 0.42f, 0.86f, 0.24f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Bell Tower Seq", "Sequence")
            .voices (10)
            .arp (2, 5, 2, 0.8f, 0.0f)
            .sampleOsc (0, smp::tubularBell, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 4500.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 3000.0f, 0.0f, 700.0f)
            .fx (FX::Delay, 0.22f, 0.28f, 0.30f, 0.85f)
            .fx (FX::Reverb, 0.46f, 0.88f, 0.22f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Crotale Sparkle Seq", "Sequence")
            .voices (12)
            .arp (0, 8, 2, 0.5f, 0.0f)
            .sampleOsc (0, smp::crotale, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 9000.0f, 0.04f, 0.0f, 1.0f, 0.55f)
            .env (0, 0.5f, 1400.0f, 0.0f, 320.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.78f, 0.58f)
            .fx (FX::Delay, 0.28f, 0.18f, 0.36f, 0.92f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Gamelan Seq", "Sequence")
            .voices (12)
            .arp (4, 7, 2, 0.5f, 0.20f)
            .sampleOsc (0, smp::gamelanGong, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.05f, 1.0f, 0.45f)
            .env (0, 0.5f, 1600.0f, 0.0f, 400.0f)
            .fx (FX::Delay, 0.24f, 0.22f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Gamelan Gong", "Bell")
            .voices (8)
            .sampleOsc (0, smp::gamelanGong, 0.9f)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 3600.0f, 0.05f, 0.05f, 1.0f, 0.35f)
            .env (0, 0.5f, 3600.0f, 0.0f, 800.0f)
            .velocity (0.6f)
            .fx (FX::Reverb, 0.44f, 0.88f, 0.24f, 1.0f)
            .master (-7.0f)
            .done());

        // The beat between two partials four hertz apart takes seconds to
        // become audible, so this only works as a very long note.
        list.push_back (Build ("Singing Bowl", "Ambient")
            .voices (6)
            .sampleOsc (0, smp::singingBowl, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.04f, 0.0f, 1.0f, 0.35f)
            .env (0, 20.0f, 4000.0f, 0.30f, 2000.0f)
            .fx (FX::Reverb, 0.55f, 0.93f, 0.20f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Steel Tongue Seq", "Sequence")
            .voices (12)
            .arp (2, 7, 2, 0.55f, 0.18f)
            .sampleOsc (0, smp::steelTongue, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4600.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1400.0f, 0.0f, 320.0f)
            .fx (FX::Delay, 0.24f, 0.20f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Steel Tongue Keys", "Bell")
            .voices (10)
            .sampleOsc (0, smp::steelTongue, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.04f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 2800.0f, 0.0f, 500.0f)
            .velocity (0.6f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-6.0f)
            .done());

        // -- plucked, sequenced -------------------------------------------------

        list.push_back (Build ("Mandolin", "Plucked")
            .voices (10)
            .sampleOsc (0, smp::mandolin, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1600.0f, 0.0f, 220.0f)
            .velocity (0.55f)
            .fx (FX::Eq, 1.0f, 0.52f, 0.66f, 0.45f)
            .fx (FX::Reverb, 0.22f, 0.60f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        // A mandolin tremolo is how the instrument sustains at all: the same
        // note struck over and over, faster than the ear separates.
        list.push_back (Build ("Mandolin Tremolo", "Sequence")
            .voices (12)
            .arp (6, 9, 1, 0.85f, 0.0f)
            .sampleOsc (0, smp::mandolin, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 500.0f, 0.0f, 140.0f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Oud", "Plucked")
            .voices (10)
            .sampleOsc (0, smp::oud, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 2400.0f, 0.0f, 320.0f)
            .velocity (0.55f)
            .bendRange (2)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Oud Seq", "Sequence")
            .voices (12)
            .arp (4, 7, 1, 0.5f, 0.22f)
            .sampleOsc (0, smp::oud, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 3600.0f, 0.06f, 0.05f, 1.0f, 0.45f)
            .env (0, 1.0f, 900.0f, 0.0f, 200.0f)
            .fx (FX::Delay, 0.22f, 0.20f, 0.30f, 0.80f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Guzheng Seq", "Sequence")
            .voices (12)
            .arp (2, 8, 2, 0.5f, 0.12f)
            .sampleOsc (0, smp::guzheng, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6500.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1200.0f, 0.0f, 260.0f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.34f, 0.88f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Guzheng Roll", "Arp")
            .voices (12)
            .arp (2, 9, 3, 0.7f, 0.0f)
            .sampleOsc (0, smp::guzheng, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5500.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 900.0f, 0.0f, 220.0f)
            .fx (FX::Delay, 0.22f, 0.20f, 0.30f, 0.82f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        // -- mixed mallet sequences ---------------------------------------------
        //
        // Two struck sources an octave apart, arpeggiated: the pattern reads as
        // one instrument with a very wide range rather than as two parts.

        list.push_back (Build ("Mallet Stack Seq", "Sequence")
            .voices (14)
            .arp (2, 7, 2, 0.5f, 0.14f)
            .sampleOsc (0, smp::vibraphone, 0.55f)
            .sampleOsc (1, smp::glockenspiel, 0.30f)
            .tune (1, 1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1200.0f, 0.0f, 280.0f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.34f, 0.88f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Kalimba Bell Seq", "Sequence")
            .voices (14)
            .arp (3, 7, 2, 0.5f, 0.20f)
            .sampleOsc (0, smp::kalimba, 0.55f)
            .sampleOsc (1, smp::celesta, 0.28f)
            .tune (1, 1)
            .filter (flt::LP24, 6500.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1000.0f, 0.0f, 240.0f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.34f, 0.88f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // A pattern on the delay send as well as the arpeggiator, so the
        // repeats come and go across the bar rather than sitting under it.
        list.push_back (Build ("Motion Mallet Seq", "Sequence")
            .voices (14)
            .arp (0, 7, 2, 0.45f, 0.12f)
            .sampleOsc (0, smp::vibraphone, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1000.0f, 0.0f, 240.0f)
            .motion (0, 6, { 0.1f, 0.8f, 0.2f, 1.0f, 0.1f, 0.6f, 0.3f, 0.9f }, 0.2f)
            .route (Src::Motion1, Dst::Fx1Mix, 0.6f)
            .motion (1, 7, { 1.0f, 0.5f, 0.8f, 0.4f, 1.0f, 0.6f, 0.7f, 0.3f }, 0.3f)
            .route (Src::Motion2, Dst::FilterCutoff, 0.25f)
            .fx (FX::Delay, 0.08f, 0.22f, 0.34f, 0.90f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // -- voices and air -----------------------------------------------------

        list.push_back (Build ("Choir Ee Pad", "Pad")
            .voices (10)
            .sampleOsc (0, smp::choirEe, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 3400.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 300.0f, 2400.0f, 0.88f, 1500.0f)
            .lfoFree (0, 0, 4.8f)
            .lfoShape (0, 1300.0f, 0.22f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Chorus, 0.28f, 0.09f, 0.38f, 0.3f)
            .fx (FX::Reverb, 0.48f, 0.90f, 0.22f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Choir Vowel Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::choirEe, 0.50f, true)
            .osc (1, wt::Formant, 0.28f, 0.28f)
            .tune (1, 0)
            .unison (1, 3, 0.09f, 0.5f, 0.85f, 0.25f)
            .filter (flt::LP24, 2800.0f, 0.06f, 0.03f, 1.0f, 0.55f)
            .env (0, 40.0f, 1800.0f, 0.80f, 900.0f)
            .fx (FX::Delay, 0.18f, 0.30f, 0.28f, 0.72f)
            .fx (FX::Reverb, 0.46f, 0.88f, 0.23f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Whistle Lead", "Lead")
            .voices (1, 1)
            .glide (30.0f, 2)
            .sampleOsc (0, smp::whistle, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.5f)
            .env (0, 25.0f, 700.0f, 0.88f, 200.0f)
            .lfoFree (0, 0, 5.4f)
            .lfoShape (0, 800.0f, 0.2f)
            .routeVia (Src::Lfo1, Src::ModWheel, Dst::Osc1Pitch, 0.020f, true)
            .fx (FX::Delay, 0.24f, 0.26f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Bottle Blow", "Keys")
            .voices (8)
            .sampleOsc (0, smp::bottleBlow, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 45.0f, 900.0f, 0.85f, 300.0f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.30f, 0.82f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Bottle Seq", "Sequence")
            .voices (10)
            .arp (4, 7, 2, 0.45f, 0.20f)
            .sampleOsc (0, smp::bottleBlow, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 2400.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 6.0f, 400.0f, 0.0f, 140.0f)
            .fx (FX::Delay, 0.26f, 0.20f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        // -- individual sounds ---------------------------------------------------

        // The pitch rises rather than falls, which is what makes a drop read as
        // a drop. Sequenced, it turns into a rhythm nothing else can make.
        list.push_back (Build ("Water Drop Seq", "Sequence")
            .voices (10)
            .arp (5, 7, 2, 0.4f, 0.18f)
            .sampleOsc (0, smp::waterDrop, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 0.5f, 500.0f, 0.0f, 140.0f)
            .fx (FX::Delay, 0.28f, 0.20f, 0.34f, 0.90f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Ice Texture", "Ambient")
            .voices (6)
            .sampleOsc (0, smp::iceCrackle, 0.60f, true)
            .osc (1, wt::BasicShapes, wt::sine, 0.22f)
            .tune (1, -1)
            .filter (flt::LP24, 9000.0f, 0.05f, 0.0f, 1.0f, 0.3f)
            .env (0, 800.0f, 3000.0f, 0.85f, 2200.0f)
            .fx (FX::Reverb, 0.55f, 0.93f, 0.20f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Reverse Swell", "FX")
            .voices (4)
            .sampleOsc (0, smp::reverseSwell, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 9000.0f, 0.05f, 0.05f, 1.0f, 0.3f)
            .env (0, 1.0f, 3000.0f, 0.6f, 400.0f)
            .fx (FX::Reverb, 0.40f, 0.86f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Sub Drop", "FX")
            .voices (2, 1)
            .sampleOsc (0, smp::subDrop, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 900.0f, 0.05f, 0.15f, 1.0f, 0.2f)
            .env (0, 1.0f, 3000.0f, 0.5f, 300.0f)
            .fx (FX::Eq, 1.0f, 0.62f, 0.38f, 0.30f)
            .master (-6.0f)
            .done());

        // -- hybrids -------------------------------------------------------------

        list.push_back (Build ("Vibe Sparkle Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::vibraphone, 0.40f)
            .osc (1, wt::BasicShapes, wt::saw, 0.40f)
            .unison (1, 7, 0.15f, 0.55f, 0.90f, 0.12f)
            .filter (flt::LP24, 3200.0f, 0.09f, 0.05f, 1.0f, 0.80f)
            .env (0, 3.0f, 1200.0f, 0.68f, 460.0f)
            .env (1, 1.0f, 240.0f, 0.0f, 240.0f)
            .routeCurved (Src::Env2, Dst::FilterCutoff, 0.28f, 0.4f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.58f)
            .fx (FX::Delay, 0.26f, 0.28f, 0.36f, 0.90f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Gong Pad", "Pad")
            .voices (8)
            .sampleOsc (0, smp::gamelanGong, 0.55f)
            .osc (1, wt::Formant, 0.30f, 0.28f)
            .tune (0, -1)
            .tune (1, 0)
            .unison (1, 5, 0.11f, 0.55f, 0.9f, 0.3f)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.0f, 1.0f, 0.35f)
            .env (0, 400.0f, 3000.0f, 0.75f, 2000.0f)
            .lfoFree (0, 0, 0.09f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.16f, true)
            .fx (FX::Reverb, 0.54f, 0.92f, 0.20f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Bowl Drone Pad", "Ambient")
            .voices (6)
            .sampleOsc (0, smp::singingBowl, 0.60f)
            .osc (1, wt::BasicShapes, wt::sine, 0.28f)
            .tune (1, -1)
            .noise (6, 0.10f)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.0f, 1.0f, 0.3f)
            .env (0, 1000.0f, 3600.0f, 0.85f, 2600.0f)
            .fx (FX::Reverb, 0.58f, 0.94f, 0.20f, 1.0f)
            .master (-9.0f)
            .done());

        return list;
    }
}
