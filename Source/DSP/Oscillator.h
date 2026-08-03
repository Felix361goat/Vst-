#pragma once

#include <array>
#include <juce_core/juce_core.h>

#include "DSP/Wavetable.h"

namespace nog::dsp
{
    /**
        A unison wavetable oscillator.

        Reads from a Wavetable owned elsewhere - the tables are immutable and
        shared between every voice, so the oscillator only holds a pointer.

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

        static constexpr int maxUnison = 16;

        struct Settings
        {
            float morph        = 0.0f;   // 0..1 across the table's frames
            int   warpMode     = 0;
            float warpAmount   = 0.0f;   // 0..1
            int   unisonVoices = 1;      // 1..maxUnison
            float detune       = 0.2f;   // 0..1
            float blend        = 0.5f;   // 0..1, centre versus side voices
            float width        = 0.5f;   // 0..1, stereo spread of side voices
            float phase        = 0.0f;   // 0..1, start phase
            float phaseRandom  = 1.0f;   // 0..1
            float pan          = 0.0f;   // -1..1
            float level        = 0.75f;  // 0..1
        };

        void prepare (double newSampleRate) noexcept;

        /** Clears phases without producing a note-on. */
        void reset() noexcept;

        /** Seeds the unison phases for a new note. */
        void noteOn() noexcept;

        /** The table to read. Not owned; must outlive the oscillator. */
        void setTable (const Wavetable* newTable) noexcept { table = newTable; }

        void setSettings (const Settings& s) noexcept
        {
            settings = s;
            layoutDirty = true;
        }

        void setFrequency (float hz) noexcept { frequency = juce::jmax (0.0f, hz); }

        /** Adds this oscillator's output into @p left and @p right.

            Accumulating rather than returning lets a voice sum its sources
            without a temporary per source.
        */
        void addNextSample (float& left, float& right) noexcept;

    private:
        struct UnisonVoice
        {
            double phase       = 0.0;
            float  detuneRatio = 1.0f;
            float  gainLeft    = 0.0f;
            float  gainRight   = 0.0f;
        };

        /** Recomputes detune ratios and per-voice panning. Only runs when the
            settings have actually changed - it costs a handful of transcendental
            calls per unison voice, which is far too much to pay per sample. */
        void updateUnisonLayout() noexcept;

        /** Reads the table at @p phase, applying the selected warp. */
        float warpedSample (double phase, double increment) const noexcept;

        float readTable (double phase, double increment) const noexcept;

        const Wavetable* table = nullptr;

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
