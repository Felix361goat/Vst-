#pragma once

#include <array>
#include <juce_core/juce_core.h>

#include "DSP/Sample.h"
#include "DSP/Wavetable.h"

namespace nog::dsp
{
    /**
        A unison oscillator that plays either a wavetable or a sample.

        Both modes share one phase accumulator. In wavetable mode the phase runs
        over a single cycle; in sample mode it runs over the whole file. That is
        what lets unison, detune, stereo width and every warp mode work
        identically on a loaded sample without a second code path.

        Reads from a Wavetable or Sample owned elsewhere - both are immutable
        once loaded and shared between every voice, so the oscillator only holds
        a pointer.

        Each unison voice picks its own mip level from its own detuned
        frequency, which is what keeps a wide detuned stack clean: the voices at
        the top of the spread are band-limited harder than the ones at the
        bottom, exactly as they should be.

        Warp applies a distortion to the phase before the table is read, so
        every mode works on every table.
    */
    class Oscillator
    {
    public:
        // Order must match params::choices::warpModes().
        enum class Warp { Off = 0, Sync, BendPlus, BendMinus, Pwm, Mirror, Asymmetric, Quantize };

        /** Order must match params::choices::oscModes(). */
        enum class Mode { Wavetable = 0, Sample };

        /** Order must match params::choices::sampleLoopModes(). */
        enum class Loop { OneShot = 0, Forward };

        static constexpr int maxUnison = 16;

        struct Settings
        {
            int   mode         = 0;      // Mode::Wavetable or Mode::Sample

            /** In wavetable mode this is the position across the table's
                frames; in sample mode it is the playback start offset, which
                is the same control doing the analogous job. */
            float morph        = 0.0f;   // 0..1
            int   warpMode     = 0;
            float warpAmount   = 0.0f;   // 0..1
            int   unisonVoices = 1;      // 1..maxUnison
            float detune       = 0.2f;   // 0..1
            float blend        = 0.5f;   // 0..1, centre versus side voices
            float width        = 0.5f;   // 0..1, stereo spread of side voices

            /** How far the unison stack fans out across the table's frames.
                Detune alone gives every voice the same timbre at a different
                pitch; this gives them different timbres too, which is what
                makes a wide stack move rather than merely beat. */
            float tableSpread  = 0.0f;   // 0..1
            float phase        = 0.0f;   // 0..1, start phase
            float phaseRandom  = 1.0f;   // 0..1
            float pan          = 0.0f;   // -1..1
            float level        = 0.75f;  // 0..1

            // -- sample mode only -------------------------------------------
            int   loop         = 0;      // Loop::OneShot or Loop::Forward
            int   rootNote     = 60;     // MIDI note the sample plays back untransposed
        };

        void prepare (double newSampleRate) noexcept;

        /** Clears phases without producing a note-on. */
        void reset() noexcept;

        /** Seeds the unison phases for a new note. */
        void noteOn() noexcept;

        /** The table to read. Not owned; must outlive the oscillator. */
        void setTable (const Wavetable* newTable) noexcept { table = newTable; }

        /** The sample to read in sample mode. Not owned - the library keeps it
            alive for the lifetime of the plugin. */
        void setSample (const Sample* newSample) noexcept { sample = newSample; }

        /** True when sample mode is selected and a sample is actually loaded. */
        bool isPlayingSample() const noexcept
        {
            return static_cast<Mode> (settings.mode) == Mode::Sample && sample != nullptr;
        }

        void setSettings (const Settings& s) noexcept
        {
            settings = s;
            layoutDirty = true;
        }

        /** Points the oscillator at the source a note is about to start on.

            noteOn() has to know which source it is starting: a wavetable begins
            at a random phase, a sample begins at its start offset. The per-block
            settings update runs after the note has already been triggered, which
            is too late - a sample note-on that thinks it is a wavetable starts
            at a random point in the file, which for a decaying one-shot means a
            random and usually very quiet fragment of it.
        */
        void prepareSourceForNoteOn (const Sample* newSample, int newMode, int newLoop,
                                     int newRootNote, float startOffset) noexcept
        {
            sample            = newSample;
            settings.mode     = newMode;
            settings.loop     = newLoop;
            settings.rootNote = newRootNote;
            settings.morph    = startOffset;
        }

        void setFrequency (float hz) noexcept { frequency = juce::jmax (0.0f, hz); }

        /** Adds this oscillator's output into @p left and @p right.

            Accumulating rather than returning lets a voice sum its sources
            without a temporary per source.
        */
        void addNextSample (float& left, float& right) noexcept;

        /** One point of the shape this oscillator is currently making, for
            drawing. Reads the top mip level and applies the warp, so what the
            editor draws is what the voices play rather than an approximation
            of it. Not for the audio thread: it ignores unison and panning. */
        float previewAt (double phase) const noexcept
        {
            return warpedSample (phase, 1.0e-4, 0, framePosition);
        }

    private:
        struct UnisonVoice
        {
            double phase       = 0.0;
            float  detuneRatio = 1.0f;
            float  gainLeft    = 0.0f;
            float  gainRight   = 0.0f;

            /** Frame this voice reads, in table frames rather than 0..1. */
            float  frame       = 0.0f;

            /** Set when a one-shot sample has played past its end. */
            bool   finished    = false;
        };

        /** Recomputes detune ratios and per-voice panning. Only runs when the
            settings have actually changed - it costs a handful of transcendental
            calls per unison voice, which is far too much to pay per sample. */
        void updateUnisonLayout() noexcept;

        /** Reads the source at @p phase, applying the selected warp. */
        float warpedSample (double phase, double increment, int channel, float frame) const noexcept;

        float readTable (double phase, double increment, float frame) const noexcept;

        /** Phase increment per output sample for one unison voice. In sample
            mode this is a fraction of the file rather than of a cycle, which is
            what lets both modes share the same phase accumulator. */
        double incrementFor (float voiceFrequency) const noexcept;

        /** One sample of the source at @p phase, before warping. */
        float readSource (double phase, double increment, int channel, float frame) const noexcept;

        const Wavetable* table  = nullptr;
        const Sample*    sample = nullptr;

        double   sampleRate = 44100.0;
        float    frequency  = 440.0f;
        Settings settings;

        std::array<UnisonVoice, maxUnison> unison {};
        int          activeUnisonVoices = 1;
        float        unisonNormalise    = 1.0f;
        float        framePosition      = 0.0f;
        bool         layoutDirty        = true;
        juce::Random random;
    };
}
