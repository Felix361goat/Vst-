#include "DSP/SampleBank.h"

#include <cmath>
#include <functional>

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

        // -- physically modelled instruments --------------------------------
        //
        // Everything above is a noise or a blip: material a wavetable cannot
        // be. What follows is the other thing a wavetable cannot be, and the
        // reason every pluck patch used to sound like a relative of every
        // other one - a string or a bar whose partials are not in an exact
        // harmonic series and do not all decay at the same rate.
        //
        // These are modelled rather than recorded for the same reasons as the
        // rest of the bank: no download, no licensing, identical everywhere.

        /** MIDI note to Hz, for writing generator pitches as note numbers. */
        float noteHz (int note)
        {
            return 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
        }

        /** Fades the last few milliseconds so the end of the file is silent.
            Without this every one-shot ends on a click. */
        void fadeTail (juce::AudioBuffer<float>& buffer, double seconds = 0.03)
        {
            const auto length = buffer.getNumSamples();
            const auto fade   = juce::jmin (length, lengthFor (seconds));
            auto* out = buffer.getWritePointer (0);

            for (int i = 0; i < fade; ++i)
                out[length - fade + i] *= 1.0f - static_cast<float> (i) / static_cast<float> (fade);
        }

        /**
            Karplus-Strong plucked string.

            A delay line one period long is filled with a noise burst and then
            fed back through a lowpass. The lowpass is the whole point: it makes
            the top of the spectrum die before the bottom, so the note gets
            duller as it decays instead of merely quieter. That is the
            difference between a plucked string and a sine with an envelope on
            it, and it is why these read as an instrument.

            @param damping   loop filter coefficient; 1 is bright and ringing,
                             0.3 is a heavily muted thud
            @param feedback  loop gain, which sets the overall decay length
            @param pick      where along the string it was plucked, 0..0.5;
                             this combs a notch into the excitation
            @param drive     tanh saturation, for the electric voices
        */
        juce::AudioBuffer<float> pluckedString (float frequency, double seconds,
                                                float brightness, float damping,
                                                float feedback, float pick,
                                                int seed, float drive = 0.0f)
        {
            const auto length = lengthFor (seconds);
            const auto delay  = juce::jmax (2, juce::roundToInt (rate / frequency));

            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            std::vector<float> line (static_cast<size_t> (delay), 0.0f);

            // The excitation is a noise burst lowpassed to taste: a nylon
            // string starts far duller than a steel one.
            Noise noise (seed);
            auto lp = 0.0f;

            for (int i = 0; i < delay; ++i)
            {
                lp += brightness * (noise.next() - lp);
                line[static_cast<size_t> (i)] = lp;
            }

            // Comb the burst to place the pick. A string plucked at 1/5 of its
            // length has no 5th harmonic, which is most of what tells nylon
            // from steel from a bridge pickup.
            const auto pickDelay = juce::jlimit (1, delay - 1,
                                                 juce::roundToInt (pick * static_cast<float> (delay)));

            for (int i = delay - 1; i >= pickDelay; --i)
                line[static_cast<size_t> (i)] -= line[static_cast<size_t> (i - pickDelay)];

            auto index = 0;
            auto loopLast = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto current = line[static_cast<size_t> (index)];

                out[i] = drive > 0.0f ? std::tanh (current * (1.0f + drive * 6.0f)) / (1.0f + drive)
                                      : current;

                loopLast = damping * current + (1.0f - damping) * loopLast;
                line[static_cast<size_t> (index)] = loopLast * feedback;

                if (++index >= delay)
                    index = 0;
            }

            fadeTail (buffer);
            return buffer;
        }

        juce::AudioBuffer<float> makeNylonGuitar()
        {
            // Plucked near the middle with a fingertip: dull excitation, fast
            // loss of the highs, and the 3rd harmonic notched out.
            return pluckedString (noteHz (48), 2.6, 0.22f, 0.42f, 0.9965f, 0.33f, 4801);
        }

        juce::AudioBuffer<float> makeSteelGuitar()
        {
            // A pick near the bridge: bright burst, slow damping, long ring.
            return pluckedString (noteHz (48), 3.2, 0.70f, 0.72f, 0.9985f, 0.14f, 4802);
        }

        juce::AudioBuffer<float> makeElectricGuitar()
        {
            // Same string, driven. The saturation is gentle enough to stay
            // clean on soft notes and thicken the attack on hard ones.
            return pluckedString (noteHz (48), 3.0, 0.55f, 0.66f, 0.9988f, 0.18f, 4803, 0.45f);
        }

        juce::AudioBuffer<float> makeMutedPluck()
        {
            // Palm mute: the hand kills the loop almost immediately, leaving
            // pitch but no sustain. This is the afroswing guitar figure.
            return pluckedString (noteHz (48), 0.9, 0.45f, 0.30f, 0.988f, 0.25f, 4804, 0.2f);
        }

        juce::AudioBuffer<float> makeFingerBass()
        {
            // An octave down and heavily damped, which is what a wound string
            // played with a fingertip does.
            return pluckedString (noteHz (36), 2.4, 0.18f, 0.38f, 0.9985f, 0.30f, 4805, 0.15f);
        }

        /**
            Additive grand piano.

            Three things separate a piano from a sawtooth with a decay on it,
            and all three are here: the partials are stretched sharp by string
            stiffness so they are not exact multiples of the fundamental; the
            high partials die far sooner than the low ones; and each note is
            three strings tuned a hair apart, so the tone beats and never sits
            still.
        */
        juce::AudioBuffer<float> makeGrandPiano()
        {
            const auto length = lengthFor (3.6);
            juce::AudioBuffer<float> buffer (1, length);
            buffer.clear();

            auto* out = buffer.getWritePointer (0);

            const auto f0 = noteHz (48);
            constexpr auto numPartials = 28;
            constexpr auto inharmonicity = 0.00035f;   // string stiffness

            // A real hammer strikes about an eighth of the way along, which
            // kills every 8th partial.
            constexpr auto strikePoint = 8.0f;

            const float stringDetune[] { 0.0f, 0.6f, -0.5f };   // in cents

            for (const auto cents : stringDetune)
            {
                const auto stringF0 = f0 * std::pow (2.0f, cents / 1200.0f);

                for (int n = 1; n <= numPartials; ++n)
                {
                    const auto ratio = static_cast<float> (n)
                                     * std::sqrt (1.0f + inharmonicity * static_cast<float> (n * n));
                    const auto frequency = stringF0 * ratio;

                    if (frequency > 18000.0f)
                        break;

                    const auto strike = std::abs (std::sin (static_cast<float> (n)
                                                            * juce::MathConstants<float>::pi / strikePoint));
                    const auto amplitude = strike / static_cast<float> (n) / 3.0f;

                    // Higher partials decay faster; this ratio is what makes
                    // the tone soften into the tail.
                    const auto rateOfDecay = 1.1f + static_cast<float> (n) * 0.42f;
                    const auto increment = frequency / static_cast<float> (rate);

                    auto phase = 0.0f;

                    for (int i = 0; i < length; ++i)
                    {
                        out[i] += std::sin (phase * twoPi) * amplitude * decay (i, length, rateOfDecay);
                        phase += increment;

                        if (phase >= 1.0f)
                            phase -= 1.0f;
                    }
                }
            }

            // The hammer itself: a short filtered thump under the attack.
            Noise noise (4806);
            auto lp = 0.0f;
            const auto thump = lengthFor (0.05);

            for (int i = 0; i < thump; ++i)
            {
                lp += 0.12f * (noise.next() - lp);
                out[i] += lp * decay (i, thump, 5.0f) * 0.5f;
            }

            fadeTail (buffer, 0.1);
            return buffer;
        }

        /**
            Electric piano - a struck tine in front of a pickup.

            The bark on the attack is a high partial with its own fast decay,
            and a touch of FM on the fundamental gives the growl that shows up
            when you hit one hard.
        */
        juce::AudioBuffer<float> makeElectricPiano()
        {
            const auto length = lengthFor (3.2);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            const auto f0 = noteHz (48);
            const auto increment = f0 / static_cast<float> (rate);

            auto phase = 0.0f;
            auto modPhase = 0.0f;
            auto tinePhase = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                // Index falls away quickly, so the growl is an attack event
                // rather than a permanent colour.
                const auto index = 2.4f * decay (i, length, 14.0f);
                const auto body  = std::sin (phase * twoPi + index * std::sin (modPhase * twoPi));

                const auto tine  = std::sin (tinePhase * twoPi) * decay (i, length, 22.0f) * 0.5f;

                out[i] = (body * decay (i, length, 2.6f) + tine) * 0.75f;

                phase     += increment;
                modPhase  += increment;
                tinePhase += increment * 7.0f;

                phase     -= std::floor (phase);
                modPhase  -= std::floor (modPhase);
                tinePhase -= std::floor (tinePhase);
            }

            fadeTail (buffer, 0.08);
            return buffer;
        }

        /** A struck bar or tine: partials in no harmonic series at all. The
            @p partials are ratios to the fundamental. */
        juce::AudioBuffer<float> struckBar (float frequency, double seconds,
                                            const std::vector<float>& partials,
                                            const std::vector<float>& amplitudes,
                                            float decayShape, int seed, float noiseAmount)
        {
            const auto length = lengthFor (seconds);
            juce::AudioBuffer<float> buffer (1, length);
            buffer.clear();

            auto* out = buffer.getWritePointer (0);

            for (size_t p = 0; p < partials.size(); ++p)
            {
                const auto partialFrequency = frequency * partials[p];

                if (partialFrequency > 18000.0f)
                    continue;

                const auto increment = partialFrequency / static_cast<float> (rate);

                // The high bars ring out shortest, as they do on a real one.
                const auto rateOfDecay = decayShape * (0.8f + partials[p] * 0.5f);
                auto phase = 0.0f;

                for (int i = 0; i < length; ++i)
                {
                    out[i] += std::sin (phase * twoPi) * amplitudes[p] * decay (i, length, rateOfDecay);
                    phase += increment;
                    phase -= std::floor (phase);
                }
            }

            // The mallet or thumbnail hitting the bar.
            Noise noise (seed);
            auto lp = 0.0f;
            const auto click = lengthFor (0.02);

            for (int i = 0; i < click; ++i)
            {
                lp += 0.4f * (noise.next() - lp);
                out[i] += lp * decay (i, click, 6.0f) * noiseAmount;
            }

            fadeTail (buffer, 0.05);
            return buffer;
        }

        juce::AudioBuffer<float> makeKalimba()
        {
            // A thumb piano tine is nearly a free bar: the second mode sits
            // way above the fourth harmonic, which is the whole character.
            return struckBar (noteHz (60), 1.8,
                              { 1.0f, 4.2f, 10.8f, 20.1f },
                              { 0.85f, 0.22f, 0.09f, 0.03f },
                              2.4f, 4807, 0.35f);
        }

        juce::AudioBuffer<float> makeSteelDrum()
        {
            // A tuned pan note is built around the octave and the twelfth,
            // with enough of the odd modes left in to sound like metal.
            return struckBar (noteHz (60), 2.2,
                              { 1.0f, 2.0f, 3.0f, 4.05f, 5.4f, 6.9f },
                              { 0.7f, 0.5f, 0.3f, 0.16f, 0.09f, 0.05f },
                              1.6f, 4808, 0.4f);
        }

        juce::AudioBuffer<float> makeMarimbaBar()
        {
            // A marimba bar is undercut so the second mode lands two octaves
            // and a major third up. That interval is the instrument.
            return struckBar (noteHz (60), 1.4,
                              { 1.0f, 4.0f, 9.2f, 16.0f },
                              { 0.9f, 0.25f, 0.08f, 0.03f },
                              3.0f, 4809, 0.45f);
        }

        // -- sustained instruments ------------------------------------------
        //
        // Everything above is struck or plucked, so it decays and is played as
        // a one-shot. These do not: a bowed string or a blown pipe goes on for
        // as long as the player wants, so they are looped instead.
        //
        // A looped sample restarts at its start offset, so the loop point is
        // the whole problem: unless the file holds a whole number of cycles,
        // every wrap is a discontinuity and the note clicks at the loop rate.
        // sustainedLoop() rounds the length to an exact number of periods.

        /** Builds a seamlessly loopable tone.

            @param harmonics  amplitude of each harmonic, 1st upwards
            @param breath     filtered noise mixed in, for the wind instruments
            @param drift      slow pitch wander in cents, so it is not sterile
        */
        juce::AudioBuffer<float> sustainedLoop (float frequency, double approximateSeconds,
                                                const std::vector<float>& harmonics,
                                                float breath, float drift, int seed)
        {
            const auto period = rate / static_cast<double> (frequency);
            const auto cycles = juce::jmax (1, juce::roundToInt (rate * approximateSeconds / period));
            const auto length = static_cast<int> (std::round (period * cycles));

            juce::AudioBuffer<float> buffer (1, length);
            buffer.clear();

            auto* out = buffer.getWritePointer (0);

            // Vibrato as phase modulation rather than as resampling. The
            // modulating term is a whole number of cycles of the loop, so it
            // returns to zero at the seam and its average is zero - which keeps
            // the note at exactly the pitch it claims. Stretching time to get
            // the same wobble would drag the mean pitch flat instead.
            constexpr double driftCycles = 3.0;

            const auto driftRatio = std::pow (2.0, static_cast<double> (drift) / 1200.0) - 1.0;
            const auto driftDepth = driftRatio * static_cast<double> (frequency) * length
                                  / (rate * juce::MathConstants<double>::twoPi * driftCycles);

            for (size_t h = 0; h < harmonics.size(); ++h)
            {
                const auto harmonic = static_cast<double> (h + 1);

                if (frequency * harmonic > 18000.0f || harmonics[h] <= 0.0f)
                    continue;

                // Phase is derived from the position in the loop rather than
                // accumulated, so the last sample joins the first exactly.
                for (int i = 0; i < length; ++i)
                {
                    const auto position = static_cast<double> (i) / length;
                    const auto wobble = driftDepth
                                      * std::sin (position * juce::MathConstants<double>::twoPi * driftCycles);

                    const auto phase = (static_cast<double> (i) / period + wobble) * harmonic;

                    out[i] += static_cast<float> (std::sin (phase * juce::MathConstants<double>::twoPi))
                            * harmonics[h];
                }
            }

            if (breath > 0.0f)
            {
                Noise noise (seed);
                auto lowPass = 0.0f;

                for (int i = 0; i < length; ++i)
                {
                    lowPass += 0.25f * (noise.next() - lowPass);
                    out[i] += lowPass * breath;
                }

                // Cross-fade the noise across the seam so the loop stays clean.
                const auto blend = juce::jmin (length / 4, lengthFor (0.01));

                for (int i = 0; i < blend; ++i)
                {
                    const auto mix = static_cast<float> (i) / static_cast<float> (blend);
                    out[i] = out[i] * mix + out[length - blend + i] * (1.0f - mix);
                }
            }

            return buffer;
        }

        juce::AudioBuffer<float> makeBowedString()
        {
            // A bowed string is very nearly a sawtooth - the bow grabs and
            // releases once per cycle - with a little bow noise on top.
            return sustainedLoop (noteHz (48), 1.2,
                                  { 1.0f, 0.5f, 0.33f, 0.25f, 0.2f, 0.17f, 0.14f, 0.12f,
                                    0.11f, 0.10f, 0.09f, 0.08f, 0.07f, 0.06f, 0.05f, 0.04f },
                                  0.04f, 4.0f, 4810);
        }

        juce::AudioBuffer<float> makeBrassSection()
        {
            // Brass puts its energy in the middle harmonics rather than the
            // fundamental, which is what makes it cut through everything.
            return sustainedLoop (noteHz (48), 1.2,
                                  { 0.55f, 0.75f, 0.85f, 0.8f, 0.6f, 0.45f, 0.32f, 0.24f,
                                    0.18f, 0.14f, 0.10f, 0.07f, 0.05f, 0.03f },
                                  0.02f, 3.0f, 4811);
        }

        juce::AudioBuffer<float> makePanFlute()
        {
            // Nearly a sine with a strong second harmonic, and enough breath
            // that it reads as blown rather than synthesised.
            return sustainedLoop (noteHz (60), 1.0,
                                  { 1.0f, 0.34f, 0.10f, 0.04f, 0.02f },
                                  0.09f, 6.0f, 4812);
        }

        juce::AudioBuffer<float> makeChoirOo()
        {
            // An "oo" is a vowel with its formants low and close together, so
            // the low harmonics dominate and the top is almost empty.
            return sustainedLoop (noteHz (48), 1.4,
                                  { 1.0f, 0.62f, 0.22f, 0.30f, 0.12f, 0.05f, 0.03f, 0.02f },
                                  0.02f, 7.0f, 4813);
        }

        // -- more struck and plucked ----------------------------------------

        juce::AudioBuffer<float> makeHarp()
        {
            // Plucked close to the end of a long string: bright, and it rings
            // for a very long time because nothing is damping it.
            return pluckedString (noteHz (48), 3.4, 0.55f, 0.80f, 0.9992f, 0.09f, 4814);
        }

        juce::AudioBuffer<float> makeClavinet()
        {
            // A hammer striking a short, tightly damped string against a
            // pickup. Almost no sustain at all, which is the funk of it.
            return pluckedString (noteHz (48), 1.2, 0.85f, 0.62f, 0.991f, 0.07f, 4815, 0.35f);
        }

        /** A plucked string with a buzzing bridge: the sitar's jawari.

            The string rests on a curved bridge and rattles against it on every
            swing, which folds the loudest part of each cycle back on itself and
            throws energy up into the high harmonics. Modelled here as a soft
            clip inside the feedback loop, so the buzz decays with the note
            instead of sitting on top of it. */
        juce::AudioBuffer<float> makeSitar()
        {
            auto buffer = pluckedString (noteHz (48), 3.0, 0.65f, 0.85f, 0.9988f, 0.12f, 4816);

            const auto length = buffer.getNumSamples();
            auto* out = buffer.getWritePointer (0);

            auto previous = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto value = out[i];

                // Rattle only on the part of the swing that reaches the bridge.
                const auto buzzing = std::abs (value) > 0.25f;
                const auto rattle  = buzzing ? (value - previous) * 3.0f : 0.0f;

                previous = value;
                out[i] = std::tanh (value * 1.4f + rattle * 0.35f);
            }

            fadeTail (buffer);
            return buffer;
        }

        juce::AudioBuffer<float> makeMusicBox()
        {
            // A music box comb tooth is a free bar, so its second mode is way
            // up at 2.76 times the fundamental rather than at the octave.
            return struckBar (noteHz (72), 1.6,
                              { 1.0f, 2.76f, 5.40f, 8.93f },
                              { 0.85f, 0.30f, 0.12f, 0.05f },
                              2.0f, 4817, 0.5f);
        }

        juce::AudioBuffer<float> makeGlockenspiel()
        {
            // Same mode ratios as the music box - it is the same physics - but
            // a heavier bar, so it rings far longer and starts brighter.
            return struckBar (noteHz (72), 2.4,
                              { 1.0f, 2.76f, 5.40f, 8.93f, 13.3f },
                              { 0.8f, 0.42f, 0.22f, 0.10f, 0.04f },
                              1.1f, 4818, 0.35f);
        }

        juce::AudioBuffer<float> makeHangDrum()
        {
            // A hang is tuned so the first three modes are the fundamental, the
            // octave and the twelfth, which is why it sounds so consonant.
            return struckBar (noteHz (60), 2.6,
                              { 1.0f, 2.0f, 3.0f, 4.2f, 5.1f },
                              { 0.85f, 0.35f, 0.18f, 0.07f, 0.03f },
                              1.3f, 4819, 0.3f);
        }

        // -- sound chips ----------------------------------------------------
        //
        // What makes an old console sound like an old console is not the
        // waveform - a square wave is a square wave - but everything the
        // hardware could not do. The output was quantised to a handful of bits,
        // clocked at a rate far below the modern one, and the pitch was set by
        // a divider that could only land on certain values. Modelling those
        // limits is the whole job; modelling the waveform is trivial.

        /** Whole cycles of a waveform, quantised and clocked the way an eight-
            bit sound chip would be. Whole cycles so it loops without a click.

            @param bits      DAC resolution; the NES triangle had four
            @param holdRate  sample-and-hold rate, for the aliasing that comes
                             from a low output clock. Zero leaves it clean. */
        juce::AudioBuffer<float> chipLoop (float frequency, double approximateSeconds,
                                           const std::function<float (float)>& shape,
                                           float bits, double holdRate)
        {
            const auto period = rate / static_cast<double> (frequency);
            const auto cycles = juce::jmax (1, juce::roundToInt (rate * approximateSeconds / period));
            const auto length = static_cast<int> (std::round (period * cycles));

            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            const auto levels = std::pow (2.0f, bits) * 0.5f;
            const auto holdEvery = holdRate > 0.0 ? juce::jmax (1, juce::roundToInt (rate / holdRate)) : 1;

            auto held = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                if (i % holdEvery == 0)
                {
                    const auto phase = static_cast<float> (std::fmod (static_cast<double> (i) / period, 1.0));
                    held = bits > 0.0f ? crush (shape (phase), levels) : shape (phase);
                }

                out[i] = held;
            }

            return buffer;
        }

        float pulseAt (float phase, float duty) { return phase < duty ? 1.0f : -1.0f; }

        juce::AudioBuffer<float> makeNesTriangle()
        {
            // The NES triangle channel had sixteen steps up and sixteen down,
            // and no volume control at all. Those stair steps are the sound of
            // every bass line on the console.
            return chipLoop (noteHz (36), 0.5,
                             [] (float phase)
                             {
                                 return 1.0f - 4.0f * std::abs (phase - 0.5f);
                             },
                             4.0f, 0.0);
        }

        juce::AudioBuffer<float> makeNesPulse()
        {
            // The narrow duty setting: thin and nasal, which is how it stayed
            // audible over everything else on three other channels.
            return chipLoop (noteHz (60), 0.4,
                             [] (float phase) { return pulseAt (phase, 0.125f); },
                             4.0f, 0.0);
        }

        juce::AudioBuffer<float> makeGameBoyWave()
        {
            // The Game Boy's fourth channel played a 32-step, 4-bit waveform
            // the programmer wrote by hand. This is a hollow, reedy one.
            return chipLoop (noteHz (48), 0.5,
                             [] (float phase)
                             {
                                 return 0.7f * std::sin (phase * twoPi)
                                      + 0.4f * std::sin (phase * twoPi * 2.0f)
                                      - 0.2f * std::sin (phase * twoPi * 3.0f)
                                      + 0.1f * std::sin (phase * twoPi * 5.0f);
                             },
                             4.0f, 0.0);
        }

        juce::AudioBuffer<float> makeSidPulse()
        {
            // The SID's pulse plus its sawtooth, which is the combination every
            // C64 lead used because either one alone was too plain.
            return chipLoop (noteHz (48), 0.5,
                             [] (float phase)
                             {
                                 return 0.6f * pulseAt (phase, 0.25f) + 0.4f * (2.0f * phase - 1.0f);
                             },
                             8.0f, 0.0);
        }

        juce::AudioBuffer<float> makeAmigaSaw()
        {
            // A tracker sample: eight bits, played back off a chip that could
            // not clock much above 28 kHz, so the top octave aliases audibly.
            // That grit is the sound of the era, not a defect to design out.
            return chipLoop (noteHz (48), 0.5,
                             [] (float phase) { return 2.0f * phase - 1.0f; },
                             8.0f, 22050.0);
        }

        juce::AudioBuffer<float> makePcSpeaker()
        {
            // One bit, on or off, straight off the motherboard.
            return chipLoop (noteHz (72), 0.3,
                             [] (float phase) { return pulseAt (phase, 0.5f); },
                             1.0f, 0.0);
        }

        /** Two-operator FM with feedback: the sixteen-bit console sound.

            The generation after the square-wave chips could do FM, which is why
            everything from that era has the same glassy, slightly metallic edge
            regardless of what instrument it was meant to be. */
        juce::AudioBuffer<float> makeFmConsole()
        {
            const auto frequency = noteHz (48);
            const auto period = rate / static_cast<double> (frequency);
            const auto cycles = juce::jmax (1, juce::roundToInt (rate * 0.5 / period));
            const auto length = static_cast<int> (std::round (period * cycles));

            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            auto feedback = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto phase = static_cast<double> (i) / period;

                // The modulator runs at three times the carrier and feeds back
                // into itself, which is what turns a clean bell into a growl.
                const auto modulator = std::sin ((phase * 3.0 + feedback * 0.4) * juce::MathConstants<double>::twoPi);
                feedback = static_cast<float> (modulator);

                out[i] = static_cast<float> (std::sin ((phase + modulator * 0.6)
                                                       * juce::MathConstants<double>::twoPi));
            }

            return buffer;
        }

        // -- nostalgia ------------------------------------------------------

        /** The dial-up handshake: two carriers, a warble and a burst of noise,
            in the order a modem actually sent them. */
        juce::AudioBuffer<float> makeDialUp()
        {
            const auto length = lengthFor (2.6);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0xD1A1);
            auto lowPass = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);
                auto value = 0.0f;

                if (t < 0.5f)
                {
                    // The answer tone.
                    value = std::sin (twoPi * 2100.0f * t) * 0.6f;
                }
                else if (t < 1.4f)
                {
                    // Two tones beating, then the warble as they negotiate.
                    const auto warble = 1.0f + 0.03f * std::sin (twoPi * 9.0f * t);
                    value = (std::sin (twoPi * 1080.0f * t * warble)
                             + std::sin (twoPi * 1750.0f * t)) * 0.35f;
                }
                else
                {
                    // Handshake: broadband noise with a tone still under it.
                    lowPass += 0.35f * (noise.next() - lowPass);
                    value = lowPass * 0.7f + std::sin (twoPi * 1800.0f * t) * 0.2f;
                }

                out[i] = value;
            }

            fadeTail (buffer, 0.15);
            return buffer;
        }

        /** A tape deck losing speed: pitch and brightness both fall away. */
        juce::AudioBuffer<float> makeTapeStop()
        {
            const auto length = lengthFor (1.6);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0x7A9F);
            auto lowPass = 0.0f;
            auto phase = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (length);

                // Speed falls away as a curve, not a line: the capstan has
                // inertia, so it dies slowly and then all at once.
                const auto speed = std::pow (1.0f - t, 1.6f);

                phase += 220.0f * speed / static_cast<float> (rate);

                lowPass += (0.05f + 0.4f * speed) * (noise.next() - lowPass);

                out[i] = (std::sin (phase * twoPi) * 0.7f + lowPass * 0.25f) * speed;
            }

            fadeTail (buffer, 0.05);
            return buffer;
        }

        /** A bell telephone: two tones struck twice a second, plus the clapper. */
        juce::AudioBuffer<float> makeTelephoneBell()
        {
            const auto length = lengthFor (2.0);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);

                // The hammer swings between the two gongs twenty times a second.
                const auto strike = std::fmod (t, 0.05f) / 0.05f;
                const auto envelope = std::exp (-14.0f * strike) * std::exp (-0.9f * t);
                const auto onFirst = std::fmod (t, 0.1f) < 0.05f;

                out[i] = std::sin (twoPi * (onFirst ? 1050.0f : 1320.0f) * t) * envelope * 0.8f;
            }

            fadeTail (buffer, 0.08);
            return buffer;
        }

        /** A CRT television left on: mains hum and the flyback whistle. */
        juce::AudioBuffer<float> makeCrtHum()
        {
            const auto length = lengthFor (2.0);
            juce::AudioBuffer<float> buffer (1, length);
            auto* out = buffer.getWritePointer (0);

            Noise noise (0xC27);
            auto lowPass = 0.0f;

            for (int i = 0; i < length; ++i)
            {
                const auto t = static_cast<float> (i) / static_cast<float> (rate);

                lowPass += 0.08f * (noise.next() - lowPass);

                out[i] = std::sin (twoPi * 50.0f * t) * 0.30f
                       + std::sin (twoPi * 100.0f * t) * 0.15f
                       // 15.625 kHz: the line frequency, and the reason a room
                       // with a television in it was never actually quiet.
                       + std::sin (twoPi * 15625.0f * t) * 0.10f
                       + lowPass * 0.25f;
            }

            return buffer;
        }

        struct Definition
        {
            const char* name;
            Generator   generate;
            bool        looping;
            int         rootNote;   // the MIDI note it was generated at
            bool        pitched;    // false for the noises, whose root is a placeholder
        };

        const std::vector<Definition>& definitions()
        {
            static const std::vector<Definition> list {
                // Pitched instruments first: they are what most patches want.
                { "Nylon Guitar",    makeNylonGuitar,    false, 48, true },
                { "Steel Guitar",    makeSteelGuitar,    false, 48, true },
                { "Electric Guitar", makeElectricGuitar, false, 48, true },
                { "Muted Pluck",     makeMutedPluck,     false, 48, true },
                { "Finger Bass",     makeFingerBass,     false, 36, true },
                { "Grand Piano",     makeGrandPiano,     false, 48, true },
                { "Electric Piano",  makeElectricPiano,  false, 48, true },
                { "Kalimba",         makeKalimba,        false, 60, true },
                { "Steel Drum",      makeSteelDrum,      false, 60, true },
                { "Marimba Bar",     makeMarimbaBar,     false, 60, true },

                { "Chip Coin",     makeChipCoin,     false, 60, false },
                { "Arcade Laser",  makeArcadeLaser,  false, 60, false },
                { "Chip Power Up", makeChipPowerUp,  false, 60, false },
                { "Metal Hit",     makeMetalHit,     false, 60, false },
                { "Glass Break",   makeGlassBreak,   false, 60, false },
                { "Click Tick",    makeClickTick,    false, 60, false },
                { "Tape Hiss",     makeTapeHiss,     true,  60, false },
                { "Vinyl Crackle", makeVinylCrackle, true,  60, false },
                { "Radio Static",  makeRadioStatic,  true,  60, false },
                { "Voice Ah",      makeVoiceAh,      true,  60, false },
                { "Retro Engine",  makeRetroEngine,  true,  60, false },

                // Appended rather than slotted in beside the other instruments:
                // presets refer to these by index, so the existing ones cannot
                // move without changing what every patch plays.
                { "Harp",          makeHarp,         false, 48, true },
                { "Clavinet",      makeClavinet,     false, 48, true },
                { "Sitar",         makeSitar,        false, 48, true },
                { "Music Box",     makeMusicBox,     false, 72, true },
                { "Glockenspiel",  makeGlockenspiel, false, 72, true },
                { "Hang Drum",     makeHangDrum,     false, 60, true },
                { "Bowed String",  makeBowedString,  true,  48, true },
                { "Brass Section", makeBrassSection, true,  48, true },
                { "Pan Flute",     makePanFlute,     true,  60, true },
                { "Choir Oo",      makeChoirOo,      true,  48, true },

                { "NES Triangle",  makeNesTriangle,  true,  36, true },
                { "NES Pulse",     makeNesPulse,     true,  60, true },
                { "Game Boy Wave", makeGameBoyWave,  true,  48, true },
                { "SID Pulse",     makeSidPulse,     true,  48, true },
                { "Amiga Saw",     makeAmigaSaw,     true,  48, true },
                { "PC Speaker",    makePcSpeaker,    true,  72, true },
                { "FM Console",    makeFmConsole,    true,  48, true },

                { "Dial Up",       makeDialUp,       false, 60, false },
                { "Tape Stop",     makeTapeStop,     false, 60, false },
                { "Telephone Bell", makeTelephoneBell, false, 60, false },
                { "CRT Hum",       makeCrtHum,       true,  60, false }
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

    int SampleBank::getRootNote (int index)
    {
        if (! juce::isPositiveAndBelow (index, getCount()))
            return 60;

        return definitions()[static_cast<size_t> (index)].rootNote;
    }

    bool SampleBank::isPitched (int index)
    {
        if (! juce::isPositiveAndBelow (index, getCount()))
            return false;

        return definitions()[static_cast<size_t> (index)].pitched;
    }

    Sample::Ptr SampleBank::get (int index) const
    {
        if (! juce::isPositiveAndBelow (index, static_cast<int> (samples.size())))
            return {};

        return samples[static_cast<size_t> (index)];
    }
}
