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

        return list;
    }
}
