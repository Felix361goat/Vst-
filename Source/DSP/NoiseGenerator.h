#pragma once

#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        White, pink and brown noise from a shared uniform source.

        Pink uses the Paul Kellet filter approximation: cheap, and flat enough
        across the audible band for a synth noise layer. Brown is a leaky
        integrator, with the leak preventing the DC wander that a pure integrator
        accumulates over long notes.
    */
    class NoiseGenerator
    {
    public:
        // Order must match params::choices::noiseColours().
        enum class Colour { White = 0, Pink, Brown };

        void reset() noexcept
        {
            pinkState = {};
            brownState = 0.0f;
        }

        void setColour (Colour c) noexcept { colour = c; }

        float getNextValue() noexcept
        {
            const auto white = random.nextFloat() * 2.0f - 1.0f;

            switch (colour)
            {
                case Colour::White:
                    return white;

                case Colour::Pink:
                {
                    pinkState.b0 = 0.99886f * pinkState.b0 + white * 0.0555179f;
                    pinkState.b1 = 0.99332f * pinkState.b1 + white * 0.0750759f;
                    pinkState.b2 = 0.96900f * pinkState.b2 + white * 0.1538520f;
                    pinkState.b3 = 0.86650f * pinkState.b3 + white * 0.3104856f;
                    pinkState.b4 = 0.55000f * pinkState.b4 + white * 0.5329522f;
                    pinkState.b5 = -0.7616f * pinkState.b5 - white * 0.0168980f;

                    const auto pink = pinkState.b0 + pinkState.b1 + pinkState.b2 + pinkState.b3
                                    + pinkState.b4 + pinkState.b5 + pinkState.b6 + white * 0.5362f;

                    pinkState.b6 = white * 0.115926f;

                    // The filter bank sums to roughly 4x unity; scale back down.
                    return pink * 0.22f;
                }

                case Colour::Brown:
                    brownState = juce::jlimit (-1.0f, 1.0f, brownState * 0.998f + white * 0.02f);
                    return brownState * 3.5f;
            }

            return white;
        }

    private:
        struct PinkState
        {
            float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
        };

        Colour        colour = Colour::White;
        PinkState     pinkState;
        float         brownState = 0.0f;
        juce::Random  random;
    };
}
