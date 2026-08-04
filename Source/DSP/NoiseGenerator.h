#pragma once

#include <juce_core/juce_core.h>

#include "DSP/Sample.h"

namespace nog::dsp
{
    /**
        The noise layer.

        Three of its colours are generated - white, pink and brown - and the
        rest are played back from a recording of a texture. That split is
        deliberate. Filtered white noise is the correct answer to "what does
        pink noise sound like" and the wrong answer to "what does a noise layer
        add to a patch": a real texture has structure over time, and structure
        is what makes a noise layer sound like breath, or tape, or rain, rather
        than like a hiss with an envelope on it.

        Pink uses the Paul Kellet filter approximation: cheap, and flat enough
        across the audible band. Brown is a leaky integrator, with the leak
        preventing the DC wander a pure integrator accumulates over long notes.

        The textured colours read from a sample at their own rate, unrelated to
        the played note - a noise layer that tracked the keyboard would be an
        oscillator, not noise.
    */
    class NoiseGenerator
    {
    public:
        // Order must match params::choices::noiseColours().
        enum class Colour { White = 0, Pink, Brown, Tape, Vinyl, Radio, Wind, Rain, Breath };

        /** True for the colours that play a recording rather than filtering a
            random number. */
        static constexpr bool isTextured (Colour c) noexcept
        {
            return static_cast<int> (c) >= static_cast<int> (Colour::Tape);
        }

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            reset();
        }

        void reset() noexcept
        {
            pinkState = {};
            brownState = 0.0f;

            // A random start so several voices with the same texture do not
            // play it in lockstep, which would sound like one loud voice.
            position = random.nextDouble();
        }

        void setColour (Colour c) noexcept { colour = c; }

        /** The texture for the current colour. Not owned; the bank keeps every
            sample alive for the lifetime of the plugin. */
        void setTexture (const Sample* newTexture) noexcept { texture = newTexture; }

        float getNextValue() noexcept
        {
            if (isTextured (colour))
            {
                if (texture == nullptr || texture->getLength() < 2)
                    return 0.0f;

                const auto value = texture->read (0, position);

                position += texture->getBaseRatio (sampleRate) / texture->getLength();

                if (position >= 1.0)
                    position -= std::floor (position);

                return value;
            }

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

        const Sample* texture = nullptr;
        double        sampleRate = 44100.0;
        double        position = 0.0;
    };
}
