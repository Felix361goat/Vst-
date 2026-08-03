#pragma once

#include <array>
#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        A unison-capable oscillator with wave morphing and phase warping.

        This is the placeholder that stands in for real wavetable playback. It
        keeps the full control surface a wavetable oscillator needs - morph
        position, warp mode and amount, unison voices, detune, blend, stereo
        width, phase and phase randomisation - but generates its waves
        analytically from a small fixed bank instead of reading table frames.

        Swapping in real wavetables later means replacing sampleForWave() and
        the meaning of the morph position. Everything around it, including the
        modulation wiring and the entire parameter surface, stays as it is.

        Saw and pulse shapes are band-limited with PolyBLEP, and morphing
        crossfades between already-band-limited outputs, so the placeholder is
        clean enough to judge patches by.
    */
    class Oscillator
    {
    public:
        // Order must match params::choices::waveforms().
        enum class Wave { Sine = 0, Triangle, Saw, Square, Pulse, NoiseTable };

        // Order must match params::choices::warpModes().
        enum class Warp { Off = 0, Sync, BendPlus, BendMinus, Pwm, Mirror, Asymmetric, Quantize };

        static constexpr int numWaves      = 6;
        static constexpr int maxUnison     = 16;

        struct Settings
        {
            int   wave         = 0;
            float morph        = 0.0f;   // 0..1, position across the wave bank
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

        void setSettings (const Settings& s) noexcept { settings = s; }
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

        /** Recomputes detune ratios and per-voice panning. Cheap enough to run
            once per sample block, which is how the voice drives it. */
        void updateUnisonLayout() noexcept;

        /** One band-limited sample of a single wave from the bank. */
        float sampleForWave (int waveIndex, double phase, double increment) const noexcept;

        /** The morphed wave: a crossfade between two adjacent bank entries. */
        float morphedSample (double phase, double increment) const noexcept;

        /** Applies the selected warp and returns the resulting sample. */
        float warpedSample (double phase, double increment) const noexcept;

        static float polyBlep (double t, double dt) noexcept;
        static float noiseTableSample (double phase) noexcept;

        double      sampleRate = 44100.0;
        float       frequency  = 440.0f;
        Settings    settings;

        std::array<UnisonVoice, maxUnison> unison {};
        int          activeUnisonVoices = 1;
        float        unisonNormalise    = 1.0f;
        juce::Random random;
    };
}
