#pragma once

#include <array>
#include <cmath>
#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        A topology-preserving-transform state variable filter.

        The TPT structure stays stable when the cutoff is swept quickly, which
        matters here because cutoff is one of the most heavily modulated
        destinations in the matrix - a naive biquad with recalculated
        coefficients zippers or blows up under an audio-rate sweep.

        All modes come out of the same core computation, so switching type costs
        nothing and does not reset the filter's state.
    */
    class StateVariableFilter
    {
    public:
        // Order must match params::choices::filterTypes().
        enum class Type { LowPass12 = 0, LowPass24, HighPass12, HighPass24, BandPass12, Notch, Peak, Allpass };

        static constexpr int maxChannels = 2;

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            reset();
        }

        void reset() noexcept
        {
            for (auto& channel : sections)
                for (auto& section : channel)
                    section = {};
        }

        void setParameters (Type newType, float cutoffHz, float resonance, float drive) noexcept
        {
            type      = newType;
            driveGain = 1.0f + drive * 24.0f;
            makeUp    = 1.0f / std::sqrt (driveGain);

            // Keep the cutoff below Nyquist with a margin; tan() blows up as the
            // normalised frequency approaches 0.5.
            const auto maxCutoff = static_cast<float> (sampleRate) * 0.49f;
            const auto clamped   = juce::jlimit (10.0f, maxCutoff, cutoffHz);

            g = static_cast<float> (std::tan (juce::MathConstants<double>::pi * clamped / sampleRate));

            // k is the damping term: 2 is fully damped, approaching 0 self-oscillates.
            k = 2.0f - 1.98f * juce::jlimit (0.0f, 1.0f, resonance);

            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        bool isFourPole() const noexcept
        {
            return type == Type::LowPass24 || type == Type::HighPass24;
        }

        float processSample (int channel, float input) noexcept
        {
            jassert (juce::isPositiveAndBelow (channel, maxChannels));

            auto x = input;

            if (driveGain > 1.0f)
                x = std::tanh (x * driveGain) * makeUp;

            auto& channelSections = sections[static_cast<size_t> (channel)];

            x = processSection (channelSections[0], x);

            // The 24 dB modes are the same section run twice.
            if (isFourPole())
                x = processSection (channelSections[1], x);

            return x;
        }

    private:
        struct Section
        {
            float ic1eq = 0.0f;
            float ic2eq = 0.0f;
        };

        float processSection (Section& s, float v0) noexcept
        {
            const auto v3 = v0 - s.ic2eq;
            const auto v1 = a1 * s.ic1eq + a2 * v3;
            const auto v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;

            s.ic1eq = 2.0f * v1 - s.ic1eq;
            s.ic2eq = 2.0f * v2 - s.ic2eq;

            const auto low  = v2;
            const auto band = v1;
            const auto high = v0 - k * v1 - v2;

            switch (type)
            {
                case Type::LowPass12:
                case Type::LowPass24:   return low;
                case Type::HighPass12:
                case Type::HighPass24:  return high;
                case Type::BandPass12:  return band;
                case Type::Notch:       return low + high;
                case Type::Peak:        return low - high;
                case Type::Allpass:     return low + high - k * band;
            }

            return low;
        }

        double sampleRate = 44100.0;
        Type   type       = Type::LowPass24;

        float g  = 0.0f;
        float k  = 2.0f;
        float a1 = 1.0f;
        float a2 = 0.0f;
        float a3 = 0.0f;

        float driveGain = 1.0f;
        float makeUp    = 1.0f;

        std::array<std::array<Section, 2>, maxChannels> sections {};
    };
}
