#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace nog::dsp
{
    /**
        A safety limiter on the master bus.

        This is not a mixing tool and is not meant to be driven. It is here
        because a synthesiser can produce a signal far louder than the patch
        implies without anyone having asked for it: a filter pushed into
        self-oscillation, a resonant sweep that lands on a fundamental, sixteen
        voices of a preset written for four. Those are startling at best and
        genuinely dangerous on headphones, and no amount of care in the factory
        patches prevents a player from finding one.

        Look-ahead peak limiting rather than a clipper. The delay line holds the
        signal back by the attack time, so the gain has already come down by the
        time the peak that caused it arrives - which means it catches the
        transient instead of letting the first few milliseconds of it through,
        and it does so without the harmonics a clipper would add.

        The release is slow enough not to pump on sustained material and fast
        enough to recover between notes. Below the threshold it is a unity gain
        stage and does nothing at all, which is the normal case: a patch that
        sits at a sensible level never touches it.
    */
    class Limiter
    {
    public:
        void prepare (double newSampleRate, int numChannels)
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

            lookaheadSamples = juce::jmax (1, static_cast<int> (sampleRate * attackSeconds));

            delayLine.setSize (juce::jmax (1, numChannels), lookaheadSamples + 1, false, true, true);

            // Peak tracking over the look-ahead window: the gain has to respond
            // to the loudest sample still to come, not the one going in.
            window.assign (static_cast<size_t> (lookaheadSamples + 1), 0.0f);

            releaseCoefficient = std::exp (-1.0f / static_cast<float> (sampleRate * releaseSeconds));

            reset();
        }

        void reset()
        {
            delayLine.clear();
            std::fill (window.begin(), window.end(), 0.0f);

            writeIndex = 0;
            gain = 1.0f;
        }

        /** Ceiling as a linear amplitude. Just under unity, so that a host
            summing this with anything else still has somewhere to go. */
        static constexpr float ceiling = 0.891f;   // -1 dBFS

        /** True while the limiter is actually pulling the level down, for the
            editor to show. */
        bool isLimiting() const noexcept { return gain < 0.999f; }

        void process (juce::AudioBuffer<float>& buffer)
        {
            const auto numChannels = juce::jmin (buffer.getNumChannels(), delayLine.getNumChannels());
            const auto numSamples  = buffer.getNumSamples();

            if (numChannels <= 0 || window.empty())
                return;

            const auto delayLength = delayLine.getNumSamples();

            for (int i = 0; i < numSamples; ++i)
            {
                auto inputPeak = 0.0f;

                for (int channel = 0; channel < numChannels; ++channel)
                    inputPeak = juce::jmax (inputPeak, std::abs (buffer.getSample (channel, i)));

                window[static_cast<size_t> (writeIndex)] = inputPeak;

                // The loudest sample anywhere in the look-ahead window, so the
                // gain is already down when that sample reaches the output.
                auto windowPeak = 0.0f;

                for (const auto value : window)
                    windowPeak = juce::jmax (windowPeak, value);

                const auto required = windowPeak > ceiling ? ceiling / windowPeak : 1.0f;

                // Attack is instant and release is slow: a limiter that eased
                // into its gain reduction would let the peak through, which is
                // the one thing it exists to prevent.
                gain = required < gain ? required
                                       : required + (gain - required) * releaseCoefficient;

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const auto delayed = delayLine.getSample (channel, writeIndex);
                    delayLine.setSample (channel, writeIndex, buffer.getSample (channel, i));
                    buffer.setSample (channel, i, delayed * gain);
                }

                if (++writeIndex >= delayLength)
                    writeIndex = 0;
            }
        }

        /** Samples of delay the look-ahead adds, to report to the host. */
        int getLatencySamples() const noexcept { return lookaheadSamples; }

    private:
        static constexpr float attackSeconds  = 0.0015f;   // 1.5 ms of look-ahead
        static constexpr float releaseSeconds = 0.060f;

        double sampleRate = 44100.0;
        int    lookaheadSamples = 64;
        int    writeIndex = 0;
        float  gain = 1.0f;
        float  releaseCoefficient = 0.999f;

        juce::AudioBuffer<float> delayLine;
        std::vector<float>       window;
    };
}
