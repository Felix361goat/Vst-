#pragma once

#include "DSP/SampleBank.h"
#include "FX/FXChain.h"
#include "Modulation/ModDefs.h"
#include "Params/ParameterIDs.h"
#include "State/FactoryPresets.h"

namespace nog::presets
{
    /**
        Fluent builder for factory patches.

        Exists so that a preset reads like a patch sheet rather than a list of
        string/float pairs. Every method writes real units - hertz, milliseconds,
        semitones - because that is how the values are reasoned about, and the
        conversion to normalised parameter space happens once at load time.

        Matrix slots and effect slots are allocated in the order they are added,
        so a preset never has to track which slot it is up to.
    */
    class Build
    {
    public:
        Build (juce::String name, juce::String category)
        {
            preset.name = std::move (name);
            preset.category = std::move (category);
        }

        // -- global ---------------------------------------------------------
        Build& master (float decibels) { return set (ids::masterGain, decibels); }

        /** @p mode 0 poly, 1 mono, 2 legato. */
        Build& voices (int count, int mode = 0)
        {
            set (ids::polyphony, static_cast<float> (count));
            return set (ids::voiceMode, static_cast<float> (mode));
        }

        /** @p mode 0 off, 1 always, 2 legato. */
        Build& glide (float milliseconds, int mode = 1)
        {
            set (ids::glideTime, milliseconds);
            return set (ids::glideMode, static_cast<float> (mode));
        }

        Build& velocity (float sensitivity) { return set (ids::velocitySens, sensitivity); }
        Build& bendRange (int semitones) { return set (ids::pitchBendRange, static_cast<float> (semitones)); }

        /** 0 off, 1 for 2x, 2 for 4x. Worth it on anything driven hard. */
        Build& oversample (int mode) { return set (ids::oversampling, static_cast<float> (mode)); }

        // -- oscillators ----------------------------------------------------
        /** @p table indexes the factory wavetables; @p morph sweeps its frames. */
        Build& osc (int index, int table, float morph, float level)
        {
            set (ids::osc (index, ids::oscEnable), 1.0f);
            set (ids::osc (index, ids::oscWave), static_cast<float> (table));
            set (ids::osc (index, ids::oscWtPos), morph);
            return set (ids::osc (index, ids::oscLevel), level);
        }

        Build& oscOff (int index) { return set (ids::osc (index, ids::oscEnable), 0.0f); }

        /** Points an oscillator at one of the built-in samples instead of a
            wavetable. @p builtIn indexes dsp::SampleBank; the root note comes
            from the bank, so a patch never has to know what pitch the sample
            was generated at. */
        Build& sampleOsc (int index, int builtIn, float level, bool loop = false,
                          float start = 0.0f)
        {
            preset.builtInSamples[static_cast<size_t> (index)] = builtIn;

            set (ids::osc (index, ids::oscEnable), 1.0f);
            set (ids::osc (index, ids::oscMode), 1.0f);

            // In sample mode this parameter is the playback start offset, and
            // its default is halfway - which is right for a wavetable and wrong
            // for a sample, where it would skip the attack entirely. A patch
            // that wants to start late says so.
            set (ids::osc (index, ids::oscWtPos), start);
            set (ids::osc (index, ids::oscSampleLoop), loop ? 1.0f : 0.0f);
            set (ids::osc (index, ids::oscSampleRoot),
                 static_cast<float> (dsp::SampleBank::getRootNote (builtIn)));
            return set (ids::osc (index, ids::oscLevel), level);
        }

        /** @p spread fans the stack across the table's frames as well as
            detuning it, so the voices differ in timbre and not only in pitch. */
        Build& unison (int index, int voices, float detune, float blend = 0.5f,
                       float width = 0.7f, float spread = 0.0f)
        {
            set (ids::osc (index, ids::oscUnison), static_cast<float> (voices));
            set (ids::osc (index, ids::oscDetune), detune);
            set (ids::osc (index, ids::oscBlend), blend);
            set (ids::osc (index, ids::oscUniSpread), spread);
            return set (ids::osc (index, ids::oscUniWidth), width);
        }

        Build& tune (int index, int octave, int semitones = 0, float cents = 0.0f)
        {
            set (ids::osc (index, ids::oscOctave), static_cast<float> (octave));
            set (ids::osc (index, ids::oscSemi), static_cast<float> (semitones));
            return set (ids::osc (index, ids::oscFine), cents);
        }

        /** @p mode 0 off, 1 sync, 2 bend+, 3 bend-, 4 PWM, 5 mirror, 6 asym, 7 quantize. */
        Build& warp (int index, int mode, float amount)
        {
            set (ids::osc (index, ids::oscWarpMode), static_cast<float> (mode));
            return set (ids::osc (index, ids::oscWarpAmount), amount);
        }

        Build& pan (int index, float position) { return set (ids::osc (index, ids::oscPan), position); }

        Build& phase (int index, float start, float randomAmount)
        {
            set (ids::osc (index, ids::oscPhase), start);
            return set (ids::osc (index, ids::oscPhaseRand), randomAmount);
        }

        Build& bypassFilter (int index) { return set (ids::osc (index, ids::oscToFilter), 0.0f); }

        // -- sub and noise --------------------------------------------------
        /** @p wave 0 sine, 1 triangle, 2 saw, 3 square. */
        Build& sub (int wave, float level, int octave = -1, bool throughFilter = false)
        {
            set (ids::subEnable, 1.0f);
            set (ids::subWave, static_cast<float> (wave));
            set (ids::subLevel, level);
            set (ids::subOctave, static_cast<float> (octave));
            return set (ids::subToFilter, throughFilter ? 1.0f : 0.0f);
        }

        /** @p colour 0 white, 1 pink, 2 brown. */
        Build& noise (int colour, float level, bool throughFilter = true)
        {
            set (ids::noiseEnable, 1.0f);
            set (ids::noiseColour, static_cast<float> (colour));
            set (ids::noiseLevel, level);
            return set (ids::noiseToFilter, throughFilter ? 1.0f : 0.0f);
        }

        // -- filter ---------------------------------------------------------
        /** @p type 0 LP12, 1 LP24, 2 HP12, 3 HP24, 4 BP12, 5 notch, 6 peak, 7 allpass. */
        Build& filter (int type, float cutoffHz, float resonance,
                       float drive = 0.0f, float mix = 1.0f, float keyTrack = 0.0f)
        {
            set (ids::filterEnable, 1.0f);
            set (ids::filterType, static_cast<float> (type));
            set (ids::filterCutoff, cutoffHz);
            set (ids::filterReso, resonance);
            set (ids::filterDrive, drive);
            set (ids::filterMix, mix);
            return set (ids::filterKeytrack, keyTrack);
        }

        Build& filterOff() { return set (ids::filterEnable, 0.0f); }

        // -- envelopes ------------------------------------------------------
        /** Times in milliseconds, sustain 0..1, curves -1..1. Envelope 1 is the
            amplitude envelope. */
        Build& env (int index, float attack, float decay, float sustain, float release,
                    float hold = 0.0f, float attackCurve = 0.0f,
                    float decayCurve = 0.0f, float releaseCurve = 0.0f)
        {
            set (ids::env (index, ids::envAttack), attack);
            set (ids::env (index, ids::envHold), hold);
            set (ids::env (index, ids::envDecay), decay);
            set (ids::env (index, ids::envSustain), sustain);
            set (ids::env (index, ids::envRelease), release);
            set (ids::env (index, ids::envAttackCurve), attackCurve);
            set (ids::env (index, ids::envDecayCurve), decayCurve);
            return set (ids::env (index, ids::envRelCurve), releaseCurve);
        }

        // -- LFOs -----------------------------------------------------------
        /** @p shape 0 sine, 1 triangle, 2 saw up, 3 saw down, 4 square,
            5 sample & hold, 6 random glide. */
        Build& lfoFree (int index, int shape, float rateHz, bool bipolar = true)
        {
            set (ids::lfo (index, ids::lfoShape), static_cast<float> (shape));
            set (ids::lfo (index, ids::lfoSyncMode), 0.0f);
            set (ids::lfo (index, ids::lfoRateHz), rateHz);
            return set (ids::lfo (index, ids::lfoBipolar), bipolar ? 1.0f : 0.0f);
        }

        /** @p division indexes tempoDivisions(): 0 is 8 bars, 5 is a quarter
            note, 7 a sixteenth. */
        Build& lfoSynced (int index, int shape, int division, bool bipolar = true)
        {
            set (ids::lfo (index, ids::lfoShape), static_cast<float> (shape));
            set (ids::lfo (index, ids::lfoSyncMode), 1.0f);
            set (ids::lfo (index, ids::lfoRateSync), static_cast<float> (division));
            return set (ids::lfo (index, ids::lfoBipolar), bipolar ? 1.0f : 0.0f);
        }

        Build& lfoShape (int index, float riseMs, float smooth, float startPhase = 0.0f)
        {
            set (ids::lfo (index, ids::lfoRise), riseMs);
            set (ids::lfo (index, ids::lfoSmooth), smooth);
            return set (ids::lfo (index, ids::lfoPhase), startPhase);
        }

        // -- modulation and effects -----------------------------------------
        /** Fills the next free matrix slot. */
        Build& route (mod::Source source, mod::Dest destination, float amount, bool bipolar = false)
        {
            if (nextModSlot >= ids::numMatrixSlots)
            {
                jassertfalse;   // preset asks for more routings than the matrix has
                return *this;
            }

            const auto slot = nextModSlot++;

            set (ids::matrix (slot, ids::modEnable), 1.0f);
            set (ids::matrix (slot, ids::modSource), static_cast<float> (source));
            set (ids::matrix (slot, ids::modDest), static_cast<float> (destination));
            set (ids::matrix (slot, ids::modAmount), amount);
            return set (ids::matrix (slot, ids::modBipolar), bipolar ? 1.0f : 0.0f);
        }

        /** As route(), but scaled by a second source rather than at full depth.

            This is how a modulator gets put under the player's control: an LFO
            routed to pitch via the mod wheel is vibrato that arrives when it is
            asked for, rather than a wobble that is always on. */
        Build& routeVia (mod::Source source, mod::Source via, mod::Dest destination,
                         float amount, bool bipolar = false)
        {
            const auto slot = nextModSlot;

            route (source, destination, amount, bipolar);

            if (slot < ids::numMatrixSlots)
                set (ids::matrix (slot, ids::modVia), static_cast<float> (via));

            return *this;
        }

        /** Fills the next free effect slot. The three controls mean different
            things per effect; see FXChain.cpp. */
        Build& fx (fx::FXChain::Type type, float mix, float a, float b, float c)
        {
            if (nextFxSlot >= ids::numFxSlots)
            {
                jassertfalse;   // preset asks for more effects than the rack has
                return *this;
            }

            const auto slot = nextFxSlot++;

            set (ids::fx (slot, ids::fxEnable), 1.0f);
            set (ids::fx (slot, ids::fxType), static_cast<float> (type));
            set (ids::fx (slot, ids::fxMix), mix);
            set (ids::fx (slot, ids::fxParamA), a);
            set (ids::fx (slot, ids::fxParamB), b);
            return set (ids::fx (slot, ids::fxParamC), c);
        }

        /** Turns the arpeggiator on. @p division indexes tempoDivisions();
            7 is a sixteenth, 6 an eighth. */
        Build& arp (int mode, int division, int octaves = 1, float gate = 0.5f, float swing = 0.0f)
        {
            set (ids::arpEnable, 1.0f);
            set (ids::arpMode, static_cast<float> (mode));
            set (ids::arpRate, static_cast<float> (division));
            set (ids::arpOctaves, static_cast<float> (octaves));
            set (ids::arpGate, gate);
            return set (ids::arpSwing, swing);
        }

        Build& macro (int index, float value) { return set (ids::macro (index), value); }

        Build& set (const juce::String& id, float value)
        {
            preset.values.emplace_back (id, value);
            return *this;
        }

        Preset done() { return std::move (preset); }

    private:
        Preset preset;
        int nextModSlot = 0;
        int nextFxSlot  = 0;
    };

    /** Convenience aliases so the preset files stay readable. */
    namespace wt
    {
        enum Table
        {
            BasicShapes = 0, HarmonicSweep, PulseWidth, Formant, FmBell,
            HardSync, WaveFolder, Organ, Growl, Digital, Bass, OddEven
        };

        // Morph positions within Basic Shapes.
        inline constexpr float sine     = 0.0f;
        inline constexpr float triangle = 0.25f;
        inline constexpr float saw      = 0.5f;
        inline constexpr float square   = 0.75f;
        inline constexpr float pulse    = 1.0f;
    }

    namespace flt
    {
        enum Type { LP12 = 0, LP24, HP12, HP24, BP12, Notch, Peak, Allpass };
    }

    using FX = fx::FXChain::Type;
    using Src = mod::Source;
    using Dst = mod::Dest;
}
