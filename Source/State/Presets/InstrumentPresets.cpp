#include "State/PresetBuilder.h"

namespace nog::presets
{
    namespace
    {
        // Indices into dsp::SampleBank, named so the patches read as
        // instruments rather than as numbers. They must stay in step with the
        // definition table in SampleBank.cpp.
        namespace smp
        {
            constexpr int nylonGuitar    = 0;
            constexpr int steelGuitar    = 1;
            constexpr int electricGuitar = 2;
            constexpr int mutedPluck     = 3;
            constexpr int fingerBass     = 4;
            constexpr int grandPiano     = 5;
            constexpr int electricPiano  = 6;
            constexpr int kalimba        = 7;
            constexpr int steelDrum      = 8;
            constexpr int marimbaBar     = 9;

            // Appended to the bank after the character samples, so these are
            // not contiguous with the block above.
            constexpr int harp           = 21;
            constexpr int clavinet       = 22;
            constexpr int sitar          = 23;
            constexpr int musicBox       = 24;
            constexpr int glockenspiel   = 25;
            constexpr int hangDrum       = 26;
            constexpr int bowedString    = 27;
            constexpr int brassSection   = 28;
            constexpr int panFlute       = 29;
            constexpr int choirOo        = 30;
        }
    }

    /**
        Patches built on the modelled instrument samples.

        These exist because a wavetable cannot be an instrument. It is one cycle
        repeated forever, so its partials are exact multiples of the fundamental
        and every one of them decays at the same rate - which is why the
        synthesised "guitar" and "piano" patches used to be so hard to tell
        apart. The samples here are struck or plucked models with stretched,
        unevenly decaying partials, and that difference is audible immediately.
    */
    std::vector<Preset> instrumentPresets()
    {
        std::vector<Preset> list;

        // -- guitars --------------------------------------------------------

        // Almost no processing on purpose: the model already has the pick
        // noise, the comb from the pick position and the decaying brightness,
        // and burying it in a filter would throw all three away.
        list.push_back (Build ("Nylon Guitar", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::nylonGuitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 3000.0f, 0.0f, 300.0f)
            .velocity (0.55f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.55f, 0.4f)
            .fx (FX::Reverb, 0.18f, 0.5f, 0.32f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Steel Guitar", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::steelGuitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 8000.0f, 0.04f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 3400.0f, 0.0f, 340.0f)
            .velocity (0.5f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.62f, 0.45f)
            .fx (FX::Reverb, 0.20f, 0.55f, 0.30f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Clean Electric", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::electricGuitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 4500.0f, 0.08f, 0.05f, 1.0f, 0.45f)
            .env (0, 1.0f, 3000.0f, 0.0f, 300.0f)
            .velocity (0.5f)
            .fx (FX::Chorus, 0.22f, 0.18f, 0.35f, 0.3f)
            .fx (FX::Delay, 0.16f, 0.28f, 0.28f, 0.75f)
            .fx (FX::Reverb, 0.22f, 0.6f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Crunch Electric", "Plucked")
            .voices (6)
            .sampleOsc (0, smp::electricGuitar, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 3200.0f, 0.14f, 0.35f, 1.0f, 0.4f)
            .env (0, 1.0f, 2600.0f, 0.0f, 260.0f)
            .fx (FX::Distortion, 0.55f, 0.42f, 0.5f, 0.5f)
            .fx (FX::Eq, 1.0f, 0.4f, 0.58f, 0.5f)
            .fx (FX::Reverb, 0.18f, 0.55f, 0.3f, 1.0f)
            .oversample (1)
            .master (-11.0f)
            .done());

        // The afroswing guitar figure: short, damped, sat far back in the mix
        // with a slapback that makes it feel played rather than programmed.
        list.push_back (Build ("Afroswing Mute", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::mutedPluck, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 3600.0f, 0.10f, 0.1f, 1.0f, 0.5f)
            .env (0, 0.5f, 700.0f, 0.0f, 140.0f)
            .velocity (0.6f)
            .fx (FX::Delay, 0.2f, 0.14f, 0.22f, 0.55f)
            .fx (FX::Reverb, 0.16f, 0.45f, 0.3f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Afroswing Mute Wide", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::mutedPluck, 0.75f)
            .osc (1, wt::BasicShapes, wt::triangle, 0.18f)
            .tune (1, 1)
            .filter (flt::LP24, 4200.0f, 0.08f, 0.08f, 1.0f, 0.5f)
            .env (0, 0.5f, 800.0f, 0.0f, 180.0f)
            .fx (FX::Chorus, 0.3f, 0.2f, 0.4f, 0.35f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.3f, 0.7f)
            .fx (FX::Reverb, 0.2f, 0.55f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        // Guitar-shaped material, played as an arpeggio: this is the sound a
        // wavetable arp cannot make, because every repeat has a real pick on it.
        list.push_back (Build ("Guitar Arp", "Arp")
            .voices (10)
            .arp (0, 7, 2, 0.55f, 0.14f)
            .sampleOsc (0, smp::nylonGuitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 1200.0f, 0.0f, 220.0f)
            .fx (FX::Delay, 0.24f, 0.18f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.26f, 0.62f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        // -- basses ---------------------------------------------------------

        list.push_back (Build ("Finger Bass", "Bass")
            .voices (2, 1)
            .sampleOsc (0, smp::fingerBass, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 1400.0f, 0.06f, 0.1f, 1.0f, 0.3f)
            .env (0, 1.0f, 2200.0f, 0.0f, 180.0f)
            .velocity (0.5f)
            .fx (FX::Eq, 1.0f, 0.55f, 0.45f, 0.35f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Afro Bass Guitar", "Bass")
            .voices (2, 1)
            .glide (35.0f, 2)
            .sampleOsc (0, smp::fingerBass, 0.85f)
            .oscOff (1)
            .sub (0, 0.35f)
            .filter (flt::LP24, 900.0f, 0.05f, 0.12f, 1.0f, 0.25f)
            .env (0, 1.0f, 2400.0f, 0.0f, 200.0f)
            .fx (FX::Eq, 1.0f, 0.6f, 0.4f, 0.35f)
            .master (-6.0f)
            .done());

        // -- pianos ---------------------------------------------------------

        list.push_back (Build ("Grand Piano", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 9000.0f, 0.03f, 0.0f, 1.0f, 0.35f)
            .env (0, 1.0f, 4000.0f, 0.0f, 400.0f)
            .velocity (0.65f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.20f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.55f, 0.35f)
            .fx (FX::Reverb, 0.22f, 0.6f, 0.3f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Soft Felt Piano", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.85f)
            .oscOff (1)
            // Rolling the top off is what a felt strip between hammer and
            // string does, and it is most of the lo-fi piano sound.
            .filter (flt::LP24, 2200.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 3.0f, 3600.0f, 0.0f, 500.0f)
            .velocity (0.7f)
            .fx (FX::Eq, 1.0f, 0.42f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.34f, 0.72f, 0.28f, 1.0f)
            .master (-5.0f)
            .done());

        list.push_back (Build ("Wide Piano Chords", "Chords")
            .voices (14)
            .sampleOsc (0, smp::grandPiano, 0.8f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.04f, 0.0f, 1.0f, 0.35f)
            .env (0, 2.0f, 4000.0f, 0.0f, 600.0f)
            .fx (FX::Chorus, 0.18f, 0.12f, 0.3f, 0.22f)
            .fx (FX::Reverb, 0.38f, 0.78f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Electric Piano", "Keys")
            .voices (12)
            .sampleOsc (0, smp::electricPiano, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.05f, 1.0f, 0.4f)
            .env (0, 1.0f, 3200.0f, 0.0f, 340.0f)
            .velocity (0.6f)
            .fx (FX::Chorus, 0.28f, 0.14f, 0.38f, 0.3f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.3f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Tremolo Rhodes", "Keys")
            .voices (12)
            .sampleOsc (0, smp::electricPiano, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.04f, 0.08f, 1.0f, 0.4f)
            .env (0, 1.0f, 3200.0f, 0.0f, 360.0f)
            // The pan wobble is the vintage tremolo: it moves the sound across
            // the stereo field rather than just changing its level.
            .lfoSynced (0, 0, 6)
            .route (Src::Lfo1, Dst::Osc1Pan, 0.45f, true)
            .fx (FX::Reverb, 0.28f, 0.65f, 0.28f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Afro Piano Keys", "Keys")
            .voices (12)
            .sampleOsc (0, smp::grandPiano, 0.75f)
            .osc (1, wt::FmBell, 0.30f, 0.18f)
            .tune (1, 1)
            .filter (flt::LP24, 5500.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 1.0f, 2600.0f, 0.0f, 320.0f)
            .fx (FX::Delay, 0.18f, 0.26f, 0.28f, 0.8f)
            .fx (FX::Reverb, 0.3f, 0.68f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        // -- tuned percussion -----------------------------------------------

        list.push_back (Build ("Kalimba", "Bell")
            .voices (10)
            .sampleOsc (0, smp::kalimba, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.04f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1800.0f, 0.0f, 250.0f)
            .velocity (0.55f)
            .fx (FX::Delay, 0.2f, 0.22f, 0.3f, 0.85f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Kalimba Seq", "Sequence")
            .voices (10)
            .arp (4, 7, 2, 0.5f, 0.16f)
            .sampleOsc (0, smp::kalimba, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 900.0f, 0.0f, 180.0f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Marimba", "Bell")
            .voices (10)
            .sampleOsc (0, smp::marimbaBar, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.04f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1400.0f, 0.0f, 200.0f)
            .velocity (0.6f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.3f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Marimba Roll", "Sequence")
            .voices (10)
            .arp (2, 8, 1, 0.6f, 0.0f)
            .sampleOsc (0, smp::marimbaBar, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 700.0f, 0.0f, 140.0f)
            .fx (FX::Reverb, 0.3f, 0.68f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Steel Pan", "Bell")
            .voices (10)
            .sampleOsc (0, smp::steelDrum, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.05f, 0.05f, 1.0f, 0.45f)
            .env (0, 0.5f, 2200.0f, 0.0f, 300.0f)
            .velocity (0.55f)
            .fx (FX::Delay, 0.18f, 0.24f, 0.26f, 0.8f)
            .fx (FX::Reverb, 0.3f, 0.7f, 0.3f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Island Pan Seq", "Sequence")
            .voices (10)
            .arp (0, 7, 2, 0.5f, 0.2f)
            .sampleOsc (0, smp::steelDrum, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5500.0f, 0.06f, 0.05f, 1.0f, 0.45f)
            .env (0, 0.5f, 800.0f, 0.0f, 180.0f)
            .fx (FX::Delay, 0.26f, 0.18f, 0.32f, 0.9f)
            .fx (FX::Reverb, 0.32f, 0.72f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        // -- hybrids --------------------------------------------------------
        //
        // A sample on one oscillator and a wavetable on the other: the sample
        // supplies an attack no wavetable can, and the wavetable supplies the
        // sustain the sample runs out of.

        list.push_back (Build ("Piano Pad Hybrid", "Pad")
            .voices (10)
            .sampleOsc (0, smp::grandPiano, 0.55f)
            .osc (1, wt::Formant, 0.35f, 0.35f)
            .tune (1, 0)
            .unison (1, 5, 0.12f, 0.55f, 0.9f)
            .filter (flt::LP24, 3600.0f, 0.06f, 0.0f, 1.0f, 0.4f)
            .env (0, 12.0f, 4000.0f, 0.55f, 1400.0f)
            .lfoFree (0, 0, 0.14f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.10f, true)
            .fx (FX::Chorus, 0.3f, 0.1f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.5f, 0.88f, 0.22f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Guitar Sparkle Hybrid", "Lead")
            .voices (8)
            .sampleOsc (0, smp::steelGuitar, 0.5f)
            .osc (1, wt::BasicShapes, wt::saw, 0.28f)
            .tune (1, 1)
            .unison (1, 5, 0.14f, 0.55f, 0.85f)
            .filter (flt::LP24, 4200.0f, 0.08f, 0.05f, 1.0f, 0.7f)
            .env (0, 2.0f, 1600.0f, 0.35f, 420.0f)
            .env (1, 1.0f, 240.0f, 0.0f, 260.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.24f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.68f, 0.55f)
            .fx (FX::Delay, 0.24f, 0.28f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.32f, 0.74f, 0.26f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Kalimba Bell Stack", "Bell")
            .voices (10)
            .sampleOsc (0, smp::kalimba, 0.6f)
            .osc (1, wt::FmBell, 0.36f, 0.3f)
            .tune (1, 1)
            .filter (flt::LP24, 6500.0f, 0.05f, 0.0f, 1.0f, 0.55f)
            .env (0, 0.5f, 2200.0f, 0.0f, 400.0f)
            .fx (FX::Delay, 0.26f, 0.22f, 0.34f, 0.9f)
            .fx (FX::Reverb, 0.38f, 0.8f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());


        // -- plucked, second wave -------------------------------------------

        list.push_back (Build ("Harp", "Plucked")
            .voices (12)
            .sampleOsc (0, smp::harp, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 7000.0f, 0.04f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 3800.0f, 0.0f, 500.0f)
            .velocity (0.55f)
            .fx (FX::Reverb, 0.30f, 0.72f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Harp Glissando", "Arp")
            .voices (12)
            .arp (2, 8, 3, 0.7f, 0.0f)
            .sampleOsc (0, smp::harp, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 1.0f, 2400.0f, 0.0f, 400.0f)
            .fx (FX::Delay, 0.20f, 0.20f, 0.30f, 0.8f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.26f, 1.0f)
            .master (-10.0f)
            .done());

        // Barely any sustain and a lot of bite: the clav is a rhythm
        // instrument that happens to have pitch.
        list.push_back (Build ("Clavinet", "Keys")
            .voices (8)
            .sampleOsc (0, smp::clavinet, 0.9f)
            .oscOff (1)
            .filter (flt::BP12, 1800.0f, 0.20f, 0.15f, 1.0f, 0.6f)
            .env (0, 0.5f, 900.0f, 0.0f, 120.0f)
            .velocity (0.65f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.25f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.62f, 0.5f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Wah Clav", "Keys")
            .voices (8)
            .sampleOsc (0, smp::clavinet, 0.9f)
            .oscOff (1)
            .filter (flt::BP12, 900.0f, 0.45f, 0.2f, 1.0f, 0.4f)
            .env (0, 0.5f, 900.0f, 0.0f, 120.0f)
            .lfoSynced (0, 0, 6)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.40f, true)
            .route (Src::ModWheel, Dst::FilterCutoff, 0.35f)
            .fx (FX::Distortion, 0.2f, 0.25f, 0.4f, 0.5f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Sitar", "Plucked")
            .voices (8)
            .sampleOsc (0, smp::sitar, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.08f, 0.05f, 1.0f, 0.45f)
            .env (0, 1.0f, 3000.0f, 0.0f, 400.0f)
            .velocity (0.5f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.66f, 0.5f)
            .fx (FX::Reverb, 0.28f, 0.68f, 0.3f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Sitar Drone Lead", "Lead")
            .voices (1, 1)
            .glide (70.0f, 2)
            .sampleOsc (0, smp::sitar, 0.8f)
            .osc (1, wt::BasicShapes, wt::sine, 0.18f)
            .tune (1, -1)
            .filter (flt::LP24, 4200.0f, 0.12f, 0.1f, 1.0f, 0.5f)
            .env (0, 2.0f, 2400.0f, 0.25f, 400.0f)
            .bendRange (12)
            .fx (FX::Delay, 0.24f, 0.26f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.28f, 1.0f)
            .master (-9.0f)
            .done());

        // -- tuned percussion, second wave ----------------------------------

        list.push_back (Build ("Music Box", "Bell")
            .voices (10)
            .sampleOsc (0, smp::musicBox, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 8000.0f, 0.04f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1600.0f, 0.0f, 260.0f)
            .velocity (0.5f)
            .fx (FX::Reverb, 0.34f, 0.74f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Music Box Lullaby", "Sequence")
            .voices (10)
            .arp (2, 6, 2, 0.6f, 0.12f)
            .sampleOsc (0, smp::musicBox, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 6000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 0.5f, 1200.0f, 0.0f, 220.0f)
            .fx (FX::Delay, 0.22f, 0.26f, 0.30f, 0.8f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Glockenspiel", "Bell")
            .voices (10)
            .sampleOsc (0, smp::glockenspiel, 0.85f)
            .oscOff (1)
            .filter (flt::LP24, 9000.0f, 0.03f, 0.0f, 1.0f, 0.55f)
            .env (0, 0.5f, 2400.0f, 0.0f, 500.0f)
            .velocity (0.5f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.72f, 0.55f)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.24f, 1.0f)
            .master (-8.0f)
            .done());

        // The glock an octave up under a wide saw is the top-line sparkle
        // trick, done with a real struck bar instead of an FM approximation.
        list.push_back (Build ("Sparkle Glock Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::glockenspiel, 0.38f)
            .osc (1, wt::BasicShapes, wt::saw, 0.42f)
            .tune (0, 1)
            .unison (1, 7, 0.16f, 0.55f, 0.9f)
            .filter (flt::LP24, 3400.0f, 0.09f, 0.06f, 1.0f, 0.8f)
            .env (0, 3.0f, 1200.0f, 0.68f, 460.0f)
            .env (1, 1.0f, 260.0f, 0.0f, 260.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.26f)
            .fx (FX::Eq, 1.0f, 0.44f, 0.74f, 0.6f)
            .fx (FX::Delay, 0.26f, 0.28f, 0.36f, 0.9f)
            .fx (FX::Reverb, 0.36f, 0.80f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        list.push_back (Build ("Hang Drum", "Bell")
            .voices (10)
            .sampleOsc (0, smp::hangDrum, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 5000.0f, 0.05f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 2600.0f, 0.0f, 400.0f)
            .velocity (0.6f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.28f, 1.0f)
            .master (-6.0f)
            .done());

        list.push_back (Build ("Hang Seq", "Sequence")
            .voices (10)
            .arp (4, 7, 1, 0.5f, 0.18f)
            .sampleOsc (0, smp::hangDrum, 0.9f)
            .oscOff (1)
            .filter (flt::LP24, 4200.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 0.5f, 1000.0f, 0.0f, 220.0f)
            .fx (FX::Delay, 0.24f, 0.20f, 0.32f, 0.85f)
            .fx (FX::Reverb, 0.34f, 0.74f, 0.26f, 1.0f)
            .master (-8.0f)
            .done());

        // -- sustained ------------------------------------------------------
        //
        // These loop, so they hold for as long as a key is down. The envelope
        // does the shaping a player would: a bow and a breath both take time to
        // reach full tone, which is why the attacks here are long.

        list.push_back (Build ("Bowed Strings", "Strings")
            .voices (10)
            .sampleOsc (0, smp::bowedString, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 3200.0f, 0.06f, 0.0f, 1.0f, 0.45f)
            .env (0, 180.0f, 1400.0f, 0.85f, 500.0f)
            .env (1, 120.0f, 900.0f, 0.6f, 400.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.20f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.15f)
            .fx (FX::Chorus, 0.22f, 0.10f, 0.32f, 0.28f)
            .fx (FX::Reverb, 0.40f, 0.82f, 0.24f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Cello Solo", "Strings")
            .voices (1, 2)
            .glide (45.0f, 2)
            .sampleOsc (0, smp::bowedString, 0.9f, true)
            .oscOff (1)
            .tune (0, -1)
            .filter (flt::LP24, 2200.0f, 0.08f, 0.05f, 1.0f, 0.4f)
            .env (0, 140.0f, 1200.0f, 0.9f, 420.0f)
            .lfoFree (0, 0, 5.2f)
            .lfoShape (0, 900.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.008f, true)
            .fx (FX::Reverb, 0.36f, 0.78f, 0.26f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("String Pad", "Pad")
            .voices (10)
            .sampleOsc (0, smp::bowedString, 0.6f, true)
            .osc (1, wt::Formant, 0.30f, 0.30f)
            .tune (1, 0)
            .unison (1, 5, 0.11f, 0.55f, 0.9f)
            .filter (flt::LP24, 2400.0f, 0.05f, 0.0f, 1.0f, 0.4f)
            .env (0, 600.0f, 2600.0f, 0.85f, 1800.0f)
            .lfoFree (0, 0, 0.12f)
            .route (Src::Lfo1, Dst::FilterCutoff, 0.14f, true)
            .fx (FX::Chorus, 0.3f, 0.08f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.52f, 0.90f, 0.22f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Brass Section", "Brass")
            .voices (8)
            .sampleOsc (0, smp::brassSection, 0.85f, true)
            .oscOff (1)
            // Brass gets brighter the harder it is blown, so the filter has to
            // open with the envelope or it sounds like a recording of a note
            // rather than a note being played.
            .filter (flt::LP24, 1600.0f, 0.10f, 0.15f, 1.0f, 0.5f)
            .env (0, 60.0f, 900.0f, 0.85f, 260.0f)
            .env (1, 90.0f, 700.0f, 0.55f, 240.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.35f)
            .route (Src::Velocity, Dst::FilterCutoff, 0.22f)
            .fx (FX::Eq, 1.0f, 0.5f, 0.6f, 0.45f)
            .fx (FX::Reverb, 0.26f, 0.66f, 0.28f, 1.0f)
            .master (-8.0f)
            .done());

        list.push_back (Build ("Brass Stab", "Brass")
            .voices (8)
            .sampleOsc (0, smp::brassSection, 0.9f, true)
            .oscOff (1)
            .filter (flt::LP24, 2000.0f, 0.14f, 0.2f, 1.0f, 0.5f)
            .env (0, 12.0f, 320.0f, 0.0f, 180.0f)
            .env (1, 6.0f, 180.0f, 0.0f, 120.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.45f)
            .fx (FX::Distortion, 0.18f, 0.22f, 0.4f, 0.5f)
            .fx (FX::Reverb, 0.24f, 0.6f, 0.3f, 1.0f)
            .master (-9.0f)
            .done());

        list.push_back (Build ("Pan Flute", "Keys")
            .voices (1, 1)
            .glide (30.0f, 2)
            .sampleOsc (0, smp::panFlute, 0.9f, true)
            .oscOff (1)
            .filter (flt::LP24, 4000.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 45.0f, 800.0f, 0.88f, 240.0f)
            .lfoFree (0, 0, 5.0f)
            .lfoShape (0, 900.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.006f, true)
            .fx (FX::Delay, 0.20f, 0.26f, 0.28f, 0.8f)
            .fx (FX::Reverb, 0.34f, 0.76f, 0.28f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Flute Sparkle Lead", "Lead")
            .voices (8)
            .sampleOsc (0, smp::panFlute, 0.5f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.32f)
            .tune (1, 0)
            .unison (1, 5, 0.13f, 0.55f, 0.88f)
            .filter (flt::LP24, 3000.0f, 0.08f, 0.05f, 1.0f, 0.75f)
            .env (0, 20.0f, 1000.0f, 0.72f, 420.0f)
            .env (1, 8.0f, 260.0f, 0.2f, 260.0f)
            .route (Src::Env2, Dst::FilterCutoff, 0.26f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.72f, 0.58f)
            .fx (FX::Delay, 0.24f, 0.30f, 0.34f, 0.85f)
            .fx (FX::Reverb, 0.38f, 0.80f, 0.24f, 1.0f)
            .master (-10.0f)
            .done());

        list.push_back (Build ("Choir Oo Pad", "Pad")
            .voices (10)
            .sampleOsc (0, smp::choirOo, 0.85f, true)
            .oscOff (1)
            .filter (flt::LP24, 2600.0f, 0.05f, 0.0f, 1.0f, 0.5f)
            .env (0, 400.0f, 2400.0f, 0.88f, 1600.0f)
            .lfoFree (0, 0, 4.6f)
            .lfoShape (0, 1400.0f, 0.2f)
            .route (Src::Lfo1, Dst::Osc1Pitch, 0.005f, true)
            .fx (FX::Chorus, 0.3f, 0.09f, 0.4f, 0.3f)
            .fx (FX::Reverb, 0.5f, 0.90f, 0.22f, 1.0f)
            .master (-7.0f)
            .done());

        list.push_back (Build ("Choir Oo Chords", "Chords")
            .voices (12)
            .sampleOsc (0, smp::choirOo, 0.6f, true)
            .osc (1, wt::BasicShapes, wt::saw, 0.24f)
            .tune (1, 1)
            .unison (1, 5, 0.12f, 0.55f, 0.9f)
            .filter (flt::LP24, 2600.0f, 0.07f, 0.04f, 1.0f, 0.62f)
            .env (0, 30.0f, 1400.0f, 0.75f, 700.0f)
            .fx (FX::Eq, 1.0f, 0.46f, 0.66f, 0.5f)
            .fx (FX::Delay, 0.18f, 0.32f, 0.28f, 0.7f)
            .fx (FX::Reverb, 0.44f, 0.86f, 0.24f, 1.0f)
            .master (-11.0f)
            .done());

        return list;
    }
}
