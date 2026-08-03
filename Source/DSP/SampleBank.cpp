#include "DSP/SampleBank.h"

#include <cmath>

namespace nog::dsp
{
    namespace
    {
        constexpr double rate  = 44100.0;
        constexpr float  twoPi = juce::MathConstants<float>::twoPi;

        /** Deterministic noise. Seeded per generator so a patch sounds the same
            every time the plugin loads, on every machine. */
        struct Noise
        {
            explicit Noise (int seed) : random (seed) {}

            float next() { return random.nextFloat() * 2.0f - 1.0f; }

            juce::Random random;
        };

        int lengthFor (double seconds) { return static_cast<int> (rate * seconds); }

        /** Quantises to a given number of levels - the sound of a cheap DAC. */
        float crush (float x, float levels)
        {
            return std::round (x * levels) / levels;
        }

        float square (float phase) { return std::fmod (phase, 1.0f) < 0.5f ? 1.0f : -1.0f; }

        /** Exponential fall from 1 to 0 across @p length. */
        float decay (int i, int length, float shape = 4.0f)
        {
            const auto t = static_cast<float> (i) / static_cast<float> (juce::jmax (1, length));
            return std::exp (-shape * t);
        }

        using Generator = juce::AudioBuffer<float> (*)();

        // -- generators -----------------------------------------------------

        /** A rising two-step blip, quantised hard. The sound of picking
            something up. */
        juce::AudioBuffer<float> makeChipCoin()
        {
            const auto length = lengthFor (0.42);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            const auto step = lengthFor (0.07);
            auto phase = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                // Two fixed pitches a fifth apart, the second held far longer.
                const auto frequency = i < step ? 988.0f : 1319.0f;
                phase += frequency / static_cast<float> (rate);

                out[i] = crush (square (phase), 8.0f) * decay (i, length, 3.2f) * 0.8f;
            }

            return buffer;
        }

        /** A descending square sweep with a bitcrushed tail. */
        juce::AudioBuffer<float> makeArcadeLaser()
        {
            const auto length = lengthFor (0.55);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            auto phase = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (length);
                const auto frequency = juce::jmap (t * t, 1800.0f, 90.0f);

                phase += frequency / static_cast<float> (rate);
                out[i] = crush (square (phase), 6.0f) * decay (i, length, 2.2f) * 0.85f;
            }

            return buffer;
        }

        /** Fast arpeggio through a chord, hard quantised: a power-up. */
        juce::AudioBuffer<float> makeChipPowerUp()
        {
            const auto length = lengthFor (0.6);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            static constexpr float notes[] { 523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f, 1568.0f };
            const auto perNote = length / static_cast<int> (std::size (notes));

            auto phase = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto index = juce::jlimit (0, static_cast<int> (std::size (notes)) - 1, i / juce::jmax (1, perNote));
                phase += notes[index] / static_cast<float> (rate);

                out[i] = crush (square (phase), 8.0f) * 0.7f * (1.0f - 0.4f * static_cast<float> (i) / static_cast<float> (length));
            }

            return buffer;
        }

        /** Inharmonic FM burst: struck metal. */
        juce::AudioBuffer<float> makeMetalHit()
        {
            const auto length = lengthFor (1.4);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            // Ratios chosen to be mutually irrational-ish, so nothing lines up
            // into a pitch and the result reads as metal rather than a note.
            static constexpr float ratios[]     { 1.0f, 2.37f, 3.71f, 5.43f, 7.19f, 9.61f };
            static constexpr float amplitudes[] { 1.0f, 0.72f, 0.55f, 0.40f, 0.28f, 0.18f };

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);
                auto value = 0.0f;

                for (size_t p = 0; p < std::size (ratios); ++p)
                {
                    // Higher partials die away faster, which is what a real bar
                    // does and most of why this sounds struck.
                    const auto partialDecay = std::exp (-(2.0f + ratios[p] * 1.6f) * t);
                    value += std::sin (twoPi * 220.0f * ratios[p] * t) * amplitudes[p] * partialDecay;
                }

                out[i] = value * 0.35f;
            }

            return buffer;
        }

        /** Dense inharmonic transient with a noise front: breaking glass. */
        juce::AudioBuffer<float> makeGlassBreak()
        {
            const auto length = lengthFor (1.1);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x51A55);
            juce::Random partials (0xB4EA6);

            std::array<float, 24> frequencies {};
            std::array<float, 24> decays {};

            for (size_t p = 0; p < frequencies.size(); ++p)
            {
                frequencies[p] = juce::jmap (partials.nextFloat(), 900.0f, 7000.0f);
                decays[p] = juce::jmap (partials.nextFloat(), 6.0f, 26.0f);
            }

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);
                auto value = noise.next() * std::exp (-90.0f * t) * 0.6f;

                for (size_t p = 0; p < frequencies.size(); ++p)
                    value += std::sin (twoPi * frequencies[p] * t) * std::exp (-decays[p] * t) * 0.08f;

                out[i] = value;
            }

            return buffer;
        }

        /** Filtered noise that wanders: tape hiss, meant to be looped. */
        juce::AudioBuffer<float> makeTapeHiss()
        {
            const auto length = lengthFor (2.0);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x7A9E);
            auto lowPass = 0.0f;
            auto wander = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);

                // A slow drift in brightness is what separates tape from plain
                // white noise.
                wander = 0.9995f * wander + 0.0005f * noise.next();
                const auto coefficient = juce::jlimit (0.05f, 0.6f, 0.25f + wander * 2.0f);

                lowPass += coefficient * (noise.next() - lowPass);
                out[i] = lowPass * (0.8f + 0.2f * std::sin (twoPi * 0.7f * t));
            }

            return buffer;
        }

        /** Sparse ticks over a quiet noise floor: vinyl crackle, looped. */
        juce::AudioBuffer<float> makeVinylCrackle()
        {
            const auto length = lengthFor (2.5);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0xC4A6);
            juce::Random ticks (0x3F17);
            auto lowPass = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                lowPass += 0.12f * (noise.next() - lowPass);
                out[i] = lowPass * 0.10f;
            }

            // Impulses with short exponential tails, scattered through.
            const auto tickCount = length / 900;

            for (int n = 0; n < tickCount; ++n)
            {
                const auto position = ticks.nextInt (length - 400);
                const auto amplitude = juce::jmap (ticks.nextFloat(), 0.2f, 1.0f);

                for (int i = 0; i < 320 && position + i < length; ++i)
                    out[position + i] += (ticks.nextFloat() * 2.0f - 1.0f)
                                       * amplitude * std::exp (-0.05f * static_cast<float> (i));
            }

            return buffer;
        }

        /** Resonant sweeps through noise: shortwave radio, looped. */
        juce::AudioBuffer<float> makeRadioStatic()
        {
            const auto length = lengthFor (2.5);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x2E7B);

            // Two resonators drifting independently through the noise.
            std::array<float, 2> z1 {}, z2 {};
            const std::array<float, 2> sweepRates { 0.13f, 0.31f };

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);
                const auto input = noise.next();
                auto value = input * 0.12f;

                for (size_t r = 0; r < z1.size(); ++r)
                {
                    const auto centre = juce::jmap (0.5f + 0.5f * std::sin (twoPi * sweepRates[r] * t),
                                                    400.0f, 3200.0f);
                    const auto g = std::tan (juce::MathConstants<float>::pi * centre / static_cast<float> (rate));
                    const auto k = 0.08f;
                    const auto a1 = 1.0f / (1.0f + g * (g + k));
                    const auto a2 = g * a1;

                    const auto v3 = input - z2[r];
                    const auto v1 = a1 * z1[r] + a2 * v3;
                    const auto v2 = z2[r] + a2 * z1[r] + g * a2 * v3;

                    z1[r] = 2.0f * v1 - z1[r];
                    z2[r] = 2.0f * v2 - z2[r];

                    value += v1 * 0.5f;   // band-pass output
                }

                out[i] = value;
            }

            return buffer;
        }

        /** A held vowel built from formants: choir-ish, looped. */
        juce::AudioBuffer<float> makeVoiceAh()
        {
            const auto length = lengthFor (2.0);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            // Formant centres for an open "ah".
            static constexpr float formants[] { 700.0f, 1220.0f, 2600.0f };
            static constexpr float widths[]   { 130.0f, 190.0f, 320.0f };
            static constexpr float gains[]    { 1.0f, 0.5f, 0.22f };

            constexpr float fundamental = 130.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);

                // A little vibrato keeps a held vowel from sounding synthetic.
                const auto pitch = fundamental * (1.0f + 0.012f * std::sin (twoPi * 4.8f * t));

                auto value = 0.0f;

                for (int harmonic = 1; harmonic <= 60; ++harmonic)
                {
                    const auto frequency = pitch * static_cast<float> (harmonic);

                    if (frequency > 12000.0f)
                        break;

                    auto amplitude = 0.0f;

                    for (size_t f = 0; f < std::size (formants); ++f)
                    {
                        const auto distance = (frequency - formants[f]) / widths[f];
                        amplitude += gains[f] * std::exp (-distance * distance);
                    }

                    value += std::sin (twoPi * frequency * t) * amplitude / static_cast<float> (harmonic);
                }

                out[i] = value * 0.5f;
            }

            return buffer;
        }

        /** Pulse wave with an unsteady rate and grit: a small engine, looped. */
        juce::AudioBuffer<float> makeRetroEngine()
        {
            const auto length = lengthFor (2.0);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x9D31);
            auto phase = 0.0f;
            auto grit = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);

                // The wobble in rate is what makes it read as mechanical rather
                // than as a held note.
                const auto frequency = 82.0f * (1.0f + 0.06f * std::sin (twoPi * 3.1f * t)
                                                     + 0.03f * std::sin (twoPi * 7.7f * t));
                phase += frequency / static_cast<float> (rate);

                grit += 0.25f * (noise.next() - grit);
                out[i] = crush (square (phase), 5.0f) * 0.55f + grit * 0.18f;
            }

            return buffer;
        }

        /** A short bright noise burst shaped like a struck ceramic tile. */
        juce::AudioBuffer<float> makeClickTick()
        {
            const auto length = lengthFor (0.25);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x1B4D);
            auto bandPass = 0.0f, previous = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);
                const auto input = noise.next();

                // Crude band-pass: difference of a one-pole and its input.
                bandPass += 0.45f * (input - bandPass);
                const auto value = bandPass - previous;
                previous = bandPass;

                out[i] = value * std::exp (-70.0f * t) * 2.2f
                       + std::sin (twoPi * 2400.0f * t) * std::exp (-140.0f * t) * 0.4f;
            }

            return buffer;
        }

        struct Definition
        {
            const char* name;
            Generator   generate;
            bool        looping;
        };

        const std::vector<Definition>& definitions()
        {
            static const std::vector<Definition> list {
                { "Chip Coin",     makeChipCoin,     false },
                { "Arcade Laser",  makeArcadeLaser,  false },
                { "Chip Power Up", makeChipPowerUp,  false },
                { "Metal Hit",     makeMetalHit,     false },
                { "Glass Break",   makeGlassBreak,   false },
                { "Click Tick",    makeClickTick,    false },
                { "Tape Hiss",     makeTapeHiss,     true  },
                { "Vinyl Crackle", makeVinylCrackle, true  },
                { "Radio Static",  makeRadioStatic,  true  },
                { "Voice Ah",      makeVoiceAh,      true  },
                { "Retro Engine",  makeRetroEngine,  true  }
            };

            return list;
        }
    }

    SampleBank::SampleBank()
    {
        for (const auto& definition : definitions())
            samples.push_back (Sample::fromBuffer (definition.name, definition.generate(), rate));
    }

    const SampleBank& SampleBank::factory()
    {
        static const SampleBank bank;
        return bank;
    }

    juce::StringArray SampleBank::getNames()
    {
        juce::StringArray names;

        for (const auto& definition : definitions())
            names.add (definition.name);

        return names;
    }

    int SampleBank::getCount()
    {
        return static_cast<int> (definitions().size());
    }

    bool SampleBank::isLooping (int index)
    {
        if (! juce::isPositiveAndBelow (index, getCount()))
            return false;

        return definitions()[static_cast<size_t> (index)].looping;
    }

    Sample::Ptr SampleBank::get (int index) const
    {
        if (! juce::isPositiveAndBelow (index, static_cast<int> (samples.size())))
            return {};

        return samples[static_cast<size_t> (index)];
    }
}
