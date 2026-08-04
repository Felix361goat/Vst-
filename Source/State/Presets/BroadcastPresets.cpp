#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        namespace smp
        {
            constexpr int stationSting = 75;
            constexpr int newsroomHit  = 76;
            constexpr int muzakBell    = 77;
            constexpr int paChime      = 78;
            constexpr int amJingle     = 79;
            constexpr int cassetteWow  = 80;
            constexpr int crowdWash    = 81;
            constexpr int rotaryDial   = 82;
            constexpr int answerBeep   = 83;
            constexpr int tapeSplice   = 84;

            constexpr int vinylCrackle = 17;
            constexpr int grandPiano   = 5;
            constexpr int vibraphone   = 57;
        }
    }

    /**
        Broadcast and tape, before everything went clean.

        The sources here are modelled rather than sampled, because an actual
        advert or television theme belongs to whoever made it and none of that
        can ship inside a plugin. What can be modelled is the equipment: the FM
        chip every jingle was written on, the transmitter that threw away
        everything below three hundred hertz, the tape the whole lot ran
        through. That is where the character came from in any case - nobody
        remembers a station ident for its melody.
    */
    std::vector<Preset> broadcastPresets()
    {
        std::vector<Preset> list;

        // -- stings and hits ---------------------------------------------------

        list.push_back (Build ("Station Sting", "Brass")
            .voices (8)
            .sampleOsc (0, smp::stationSting, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.06f, 0.10f, 1.0f, 0.5f)
            .env (0, 1.0f, 1400.0f, 0.20f, 300.0f)
            .velocity (0.5f)
            .fx (FX::Eq, 1.0f, 0.48f, 0.62f, 0.45f)
            .fx (FX::Reverb, 0.28f, 0.70f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Station Sting Wide", "Chords")
            .voices (12)
            .sampleOsc (0, smp::stationSting, 0.60f)
            .osc (1, wt::BasicShapes, wt::saw, 0.28f)
            .tune (1, 0, 0, 8.0f)
            .unison (1, 5, 0.13f, 0.55f, 0.90f, 0.15f)
            .filter (flt::LP24, 3600.0f, 0.08f, 0.06f, 1.0f, 0.55f)
            .env (0, 2.0f, 1600.0f, 0.30f, 500.0f)
            .fx (FX::Dimension, 0.45f, 0.45f, 0.30f, 0.95f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Newsroom Hit", "FX")
            .voices (10)
            .sampleOsc (0, smp::newsroomHit, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.08f, 0.15f, 1.0f, 0.45f)
            .env (0, 0.5f, 500.0f, 0.0f, 200.0f)
            .velocity (0.6f)
            .fx (FX::Eq, 1.0f, 0.50f, 0.62f, 0.45f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        // Sequenced, the orchestra hit stops being a punctuation mark and
        // becomes the riff - which is what half of nineties dance did with it.
        list.push_back (Build ("Hit Stab Seq", "Sequence")
            .voices (10)
            .arp (4, 7, 1, 0.4f, 0.14f)
            .sampleOsc (0, smp::newsroomHit, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.12f, 0.15f, 1.0f, 0.5f)
            .env (0, 0.5f, 400.0f, 0.0f, 140.0f)
            .fx (FX::Delay, 0.22f, 0.20f, 0.30f, 0.80f)
            .fx (FX::Reverb, 0.28f, 0.70f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        // -- hold music --------------------------------------------------------

        list.push_back (Build ("Muzak Bell", "Bell")
            .voices (12)
            .sampleOsc (0, smp::muzakBell, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 1.0f, 2400.0f, 0.10f, 500.0f)
            .velocity (0.5f)
            .fx (FX::Chorus, 0.24f, 0.14f, 0.34f, 0.28f)
            .fx (FX::Reverb, 0.34f, 0.78f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Hold Music Keys", "Keys")
            .voices (12)
            .sampleOsc (0, smp::muzakBell, 0.55f)
            .sampleOsc (1, smp::vibraphone, 0.35f)
            .filter (flt::LP24, 3000.0f, 0.05f, 0.0f, 1.0f, 0.42f)
            .env (0, 2.0f, 2600.0f, 0.20f, 600.0f)
            // Narrow, because a telephone line is narrow, and that is most of
            // why hold music sounds like hold music.
            .filter2 (flt::HP24, 420.0f, 0.12f)
            .fx (FX::Chorus, 0.26f, 0.12f, 0.36f, 0.3f)
            .fx (FX::Reverb, 0.30f, 0.74f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Muzak Seq", "Sequence")
            .voices (12)
            .arp (2, 7, 2, 0.55f, 0.20f)
            .sampleOsc (0, smp::muzakBell, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1000.0f, 0.0f, 240.0f)
            .fx (FX::Delay, 0.24f, 0.22f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // -- announcements -----------------------------------------------------

        list.push_back (Build ("PA Chime", "Bell")
            .voices (8)
            .sampleOsc (0, smp::paChime, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 0.5f, 2400.0f, 0.0f, 400.0f)
            // A tannoy has no bottom and no top: a band is the announcement.
            .filter2 (flt::HP24, 500.0f, 0.15f)
            .fx (FX::Reverb, 0.40f, 0.86f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Airport Chime Seq", "Sequence")
            .voices (10)
            .arp (2, 5, 1, 0.7f, 0.0f)
            .sampleOsc (0, smp::paChime, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1800.0f, 0.0f, 400.0f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.30f, 0.82f)
            .fx (FX::Reverb, 0.44f, 0.88f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Answer Machine", "FX")
            .voices (4)
            .sampleOsc (0, smp::answerBeep, 0.85f)
            .oscOff (1)
            .filter (flt::BP12, 1000.0f, 0.15f, 0.05f, 1.0f, 0.5f)
            .env (0, 0.5f, 900.0f, 0.0f, 100.0f)
            .fx (FX::Reverb, 0.18f, 0.55f, 0.32f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Rotary Dial", "FX")
            .voices (4)
            .sampleOsc (0, smp::rotaryDial, 0.9f)
            .oscOff (1)
            .filter (flt::BP12, 900.0f, 0.20f, 0.10f, 1.0f, 0.35f)
            .env (0, 0.5f, 1400.0f, 0.5f, 200.0f)
            .fx (FX::Eq, 1.0f, 0.48f, 0.50f, 0.4f)
            .fx (FX::Reverb, 0.20f, 0.58f, 0.32f, 1.0f)
            .master (-8.0f)
            .done());

        // -- radio -------------------------------------------------------------

        list.push_back (Build ("AM Jingle Lead", "Lead")
            .voices (1, 1)
            .glide (26.0f, 2)
            .sampleOsc (0, smp::amJingle, 0.85f, true)
            .oscOff (1)
            // The transmitter, modelled as a band rather than as an EQ curve:
            // outside it there is genuinely nothing.
            .filter (flt::LP24, 3000.0f, 0.12f, 0.15f, 1.0f, 0.55f)
            .filter2 (flt::HP24, 320.0f, 0.18f)
            .env (0, 3.0f, 700.0f, 0.85f, 180.0f)
            .noise (5, 0.08f)
            .fx (FX::Distortion, 0.14f, 0.20f, 0.5f, 0.5f)
            .fx (FX::Delay, 0.20f, 0.24f, 0.28f, 0.78f)
            .fx (FX::Reverb, 0.22f, 0.62f, 0.30f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("AM Jingle Seq", "Sequence")
            .voices (8)
            .arp (0, 7, 2, 0.45f, 0.16f)
            .sampleOsc (0, smp::amJingle, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 3200.0f, 0.12f, 0.12f, 1.0f, 0.55f)
            .filter2 (flt::HP24, 340.0f, 0.15f)
            .env (0, 1.0f, 500.0f, 0.0f, 140.0f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.32f, 0.86f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-10.0f)
            .done());

        // -- tape ---------------------------------------------------------------

        list.push_back (Build ("Cassette Tone", "Keys")
            .voices (8)
            .sampleOsc (0, smp::cassetteWow, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.05f, 1.0f, 0.4f)
            .env (0, 20.0f, 1400.0f, 0.80f, 400.0f)
            .noise (3, 0.07f, false)
            .fx (FX::Eq, 1.0f, 0.42f, 0.42f, 0.32f)
            .fx (FX::Reverb, 0.30f, 0.74f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        // Piano through tape, which is the whole lo-fi genre in one patch: the
        // wow is what makes it sound found rather than played.
        list.push_back (Build ("Found Tape Piano", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.75f)
            .sampleOsc (1, smp::cassetteWow, 0.18f, true)
            .filter (flt::LP24, 1900.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 3.0f, 3000.0f, 0.0f, 500.0f)
            .noise (3, 0.06f, false)
            .velocity (0.7f)
            .fx (FX::Eq, 1.0f, 0.40f, 0.38f, 0.30f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Tape Splice FX", "FX")
            .voices (6)
            .sampleOsc (0, smp::tapeSplice, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.08f, 0.10f, 1.0f, 0.35f)
            .env (0, 0.5f, 800.0f, 0.0f, 150.0f)
            .fx (FX::Reverb, 0.18f, 0.52f, 0.32f, 1.0f)
            .master (-9.0f)
            .done());

        // -- room ---------------------------------------------------------------

        list.push_back (Build ("Crowd Room", "Ambient")
            .voices (4)
            .sampleOsc (0, smp::crowdWash, 0.75f, true)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.25f)
            .env (0, 400.0f, 2400.0f, 0.85f, 1400.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.48f, 0.35f)
            .fx (FX::Reverb, 0.48f, 0.90f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Live Record Pad", "Pad")
            .voices (10)
            .osc (0, wt::Formant, 0.30f, 0.45f)
            .osc (1, wt::BasicShapes, 0.40f, 0.22f)
            .tune (1, -1)
            .unison (0, 5, 0.12f, 0.55f, 0.9f, 0.25f)
            // Crowd and vinyl underneath: the pad sounds like it was taped off
            // a broadcast rather than rendered.
            .sampleOsc (1, smp::vinylCrackle, 0.10f, true)
            .noise (4, 0.05f, false)
            .filter (flt::LP24, 2200.0f, 0.05f, 0.0f, 1.0f, 0.35f)
            .env (0, 600.0f, 2600.0f, 0.85f, 1800.0f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.44f, 0.32f)
            .fx (FX::Reverb, 0.50f, 0.90f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        return list;
    }
}
