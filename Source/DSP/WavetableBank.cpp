#include "DSP/WavetableBank.h"

#include <cmath>

namespace nog::dsp
{
    namespace
    {
        using Spectrum = Wavetable::Spectrum;

        constexpr int framesPerTable = 24;
        constexpr float pi  = juce::MathConstants<float>::pi;
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        // -- elementary shapes, phase in [0, 1) -----------------------------
        float sine (float t)      { return std::sin (t * twoPi); }
        float saw (float t)       { return 2.0f * t - 1.0f; }
        float square (float t)    { return t < 0.5f ? 1.0f : -1.0f; }
        float triangle (float t)  { return 4.0f * std::abs (t - 0.5f) - 1.0f; }
        float pulse (float t, float duty) { return t < duty ? 1.0f : -1.0f; }

        float mix (float a, float b, float amount) { return a + (b - a) * amount; }

        /** Position of a frame within the table, 0 at the first frame and 1 at
            the last. Every generator below is written in terms of this. */
        float framePosition (int frame, int total)
        {
            return total <= 1 ? 0.0f : static_cast<float> (frame) / static_cast<float> (total - 1);
        }

        /** Smoothly rolls a harmonic off around a moving limit, rather than
            cutting it dead - a hard edge in the spectrum rings. */
        float rolloff (int harmonic, float limit, float softness = 0.35f)
        {
            const auto width = juce::jmax (1.0f, limit * softness);
            return juce::jlimit (0.0f, 1.0f, (limit - static_cast<float> (harmonic)) / width);
        }

        /** Deterministic per-harmonic pseudo-random value in [0, 1). Seeded so
            a patch sounds identical every time the plugin is loaded. */
        float hashToUnit (int harmonic, int seed)
        {
            auto h = static_cast<juce::uint32> (harmonic * 374761393 + seed * 668265263);
            h = (h ^ (h >> 13)) * 1274126177u;
            return static_cast<float> (h ^ (h >> 16)) / 4294967296.0f;
        }

        // -- table generators ----------------------------------------------
        // Each returns one spectrum per frame. Smooth shapes are described in
        // the time domain and analysed; anything defined by its harmonics is
        // written as a sine series directly.

        /** Sine through triangle, saw, square and finally a narrow pulse. */
        std::vector<Spectrum> generateBasicShapes()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto position = framePosition (frame, framesPerTable) * 4.0f;
                const auto stage    = juce::jlimit (0, 3, static_cast<int> (position));
                const auto blend    = position - static_cast<float> (stage);

                frames.push_back (Wavetable::analyse ([stage, blend] (float t)
                {
                    switch (stage)
                    {
                        case 0:  return mix (sine (t), triangle (t), blend);
                        case 1:  return mix (triangle (t), saw (t), blend);
                        case 2:  return mix (saw (t), square (t), blend);
                        default: return mix (square (t), pulse (t, 0.12f), blend);
                    }
                }));
            }

            return frames;
        }

        /** A saw opening up from a pure sine: the classic filter-like sweep,
            except it happens in the table instead of the filter. */
        std::vector<Spectrum> generateHarmonicSweep()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                // Exponential so the early frames still change audibly. The
                // offset keeps the fundamental alive at the bottom of the
                // sweep - a limit of exactly 1 would roll it off to nothing.
                const auto limit = 1.5f + std::pow (2.0f, framePosition (frame, framesPerTable) * 9.0f);

                frames.push_back (Wavetable::sineSeries ([limit] (int harmonic)
                {
                    return rolloff (harmonic, limit) / static_cast<float> (harmonic);
                }));
            }

            return frames;
        }

        /** Pulse width from a square down to a thin spike. */
        std::vector<Spectrum> generatePulseWidth()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto duty = juce::jmap (framePosition (frame, framesPerTable), 0.5f, 0.03f);
                frames.push_back (Wavetable::analyse ([duty] (float t) { return pulse (t, duty); }));
            }

            return frames;
        }

        /** Two formant peaks sliding up over a saw - vocal, and the basis of
            most talking-synth patches. */
        std::vector<Spectrum> generateFormant()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto position = framePosition (frame, framesPerTable);
                const auto first    = juce::jmap (position, 3.0f, 22.0f);
                const auto second   = juce::jmap (position, 9.0f, 48.0f);

                frames.push_back (Wavetable::sineSeries ([first, second] (int harmonic)
                {
                    const auto n = static_cast<float> (harmonic);

                    const auto peak = [n] (float centre, float width, float gain)
                    {
                        const auto distance = (n - centre) / width;
                        return gain * std::exp (-distance * distance);
                    };

                    // A gentle saw underneath keeps the sound from being a pair
                    // of isolated whistles.
                    const auto body = 0.25f / n * rolloff (harmonic, 64.0f);

                    return body + peak (first, first * 0.4f + 1.0f, 1.0f) / n
                                + peak (second, second * 0.35f + 1.0f, 0.6f) / n;
                }));
            }

            return frames;
        }

        /** Frequency modulation with a rising index: sine at one end, bell and
            metallic clang at the other. */
        std::vector<Spectrum> generateFm()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto index = framePosition (frame, framesPerTable) * 7.0f;

                frames.push_back (Wavetable::analyse ([index] (float t)
                {
                    return std::sin (t * twoPi + index * std::sin (t * twoPi * 2.0f));
                }));
            }

            return frames;
        }

        /** Hard sync: the wave restarts faster and faster inside one cycle,
            sweeping a formant upwards. */
        std::vector<Spectrum> generateSync()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto ratio = 1.0f + framePosition (frame, framesPerTable) * 7.0f;

                frames.push_back (Wavetable::analyse ([ratio] (float t)
                {
                    return saw (std::fmod (t * ratio, 1.0f));
                }));
            }

            return frames;
        }

        /** A sine driven into a wavefolder: harmonics appear in bursts rather
            than smoothly, which is what gives West Coast synthesis its bite. */
        std::vector<Spectrum> generateWaveFolder()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto drive = 1.0f + framePosition (frame, framesPerTable) * 6.0f;

                frames.push_back (Wavetable::analyse ([drive] (float t)
                {
                    // Folding a sine through another sine is the cheapest
                    // continuous folder and stays perfectly band-limited-ish.
                    return std::sin (std::sin (t * twoPi) * drive * pi * 0.5f);
                }));
            }

            return frames;
        }

        /** Drawbar organ: harmonics fade in one register at a time. */
        std::vector<Spectrum> generateOrgan()
        {
            std::vector<Spectrum> frames;

            // Classic drawbar footages, as harmonic numbers.
            static constexpr int drawbars[] { 1, 2, 3, 4, 6, 8, 10, 12, 16 };

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                // Starts with the first drawbar already out, so the quietest
                // frame is a sine rather than silence.
                const auto opened = 1.0f + framePosition (frame, framesPerTable)
                                         * static_cast<float> (std::size (drawbars) - 1);

                frames.push_back (Wavetable::sineSeries ([opened] (int harmonic)
                {
                    auto amplitude = 0.0f;

                    for (size_t i = 0; i < std::size (drawbars); ++i)
                        if (drawbars[i] == harmonic)
                            amplitude += juce::jlimit (0.0f, 1.0f, opened - static_cast<float> (i));

                    return amplitude;
                }));
            }

            return frames;
        }

        /** A comb notch sliding through a saw. Rich and vocal in the low
            registers - the starting point for most growl basses. */
        std::vector<Spectrum> generateGrowl()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto depth = juce::jmap (framePosition (frame, framesPerTable), 0.02f, 0.5f);

                frames.push_back (Wavetable::sineSeries ([depth] (int harmonic)
                {
                    const auto n = static_cast<float> (harmonic);
                    const auto comb = std::abs (std::sin (n * pi * depth));

                    return comb / n * rolloff (harmonic, 200.0f);
                }));
            }

            return frames;
        }

        /** Pseudo-random harmonic content morphing between two fixed seeds -
            harsh, digital, and useful for textures and noise layers. */
        std::vector<Spectrum> generateDigital()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto blend = framePosition (frame, framesPerTable);

                frames.push_back (Wavetable::sineSeries ([blend] (int harmonic)
                {
                    const auto a = hashToUnit (harmonic, 11);
                    const auto b = hashToUnit (harmonic, 97);
                    const auto n = static_cast<float> (harmonic);

                    return mix (a, b, blend) / std::sqrt (n) * rolloff (harmonic, 128.0f);
                }));
            }

            return frames;
        }

        /** Heavy fundamental with the upper harmonics coming in gradually:
            stays solid at the bottom of the keyboard where a saw turns to mush. */
        std::vector<Spectrum> generateBass()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto bite = framePosition (frame, framesPerTable);

                frames.push_back (Wavetable::sineSeries ([bite] (int harmonic)
                {
                    const auto n = static_cast<float> (harmonic);

                    if (harmonic == 1)
                        return 1.0f;

                    // The upper harmonics stay quiet until `bite` opens them up.
                    return bite * rolloff (harmonic, juce::jmap (bite, 4.0f, 96.0f)) / std::pow (n, 1.2f);
                }));
            }

            return frames;
        }

        /** Odd harmonics only through to the full series: hollow and clarinet
            like at one end, bright and sawlike at the other. */
        std::vector<Spectrum> generateOddEven()
        {
            std::vector<Spectrum> frames;

            for (int frame = 0; frame < framesPerTable; ++frame)
            {
                const auto evens = framePosition (frame, framesPerTable);

                frames.push_back (Wavetable::sineSeries ([evens] (int harmonic)
                {
                    const auto n = static_cast<float> (harmonic);
                    const auto gain = harmonic % 2 == 1 ? 1.0f : evens;

                    return gain / n * rolloff (harmonic, 160.0f);
                }));
            }

            return frames;
        }

        /** Four exact shapes for the sub oscillator, one per frame. */
        std::vector<Spectrum> generateSubShapes()
        {
            return {
                Wavetable::analyse ([] (float t) { return sine (t); }),
                Wavetable::analyse ([] (float t) { return triangle (t); }),
                Wavetable::analyse ([] (float t) { return saw (t); }),
                Wavetable::analyse ([] (float t) { return square (t); })
            };
        }

        // -- the catalogue --------------------------------------------------
        struct TableDefinition
        {
            const char* name;
            std::vector<Spectrum> (*generate)();
        };

        const std::vector<TableDefinition>& definitions()
        {
            static const std::vector<TableDefinition> list {
                { "Basic Shapes",    generateBasicShapes },
                { "Harmonic Sweep",  generateHarmonicSweep },
                { "Pulse Width",     generatePulseWidth },
                { "Formant",         generateFormant },
                { "FM Bell",         generateFm },
                { "Hard Sync",       generateSync },
                { "Wave Folder",     generateWaveFolder },
                { "Organ",           generateOrgan },
                { "Growl",           generateGrowl },
                { "Digital",         generateDigital },
                { "Bass",            generateBass },
                { "Odd / Even",      generateOddEven }
            };

            return list;
        }
    }

    WavetableBank::WavetableBank()
    {
        tables.resize (definitions().size());

        for (size_t i = 0; i < definitions().size(); ++i)
            tables[i].build (definitions()[i].name, definitions()[i].generate());

        subTable.build ("Sub Shapes", generateSubShapes());
    }

    const WavetableBank& WavetableBank::factory()
    {
        // Built on first use and shared for the lifetime of the process. The
        // tables are immutable once built, so every voice in every instance can
        // read them concurrently without synchronisation.
        static const WavetableBank bank;
        return bank;
    }

    juce::StringArray WavetableBank::getTableNames()
    {
        juce::StringArray names;

        for (const auto& definition : definitions())
            names.add (definition.name);

        return names;
    }

    int WavetableBank::getNumTables()
    {
        return static_cast<int> (definitions().size());
    }

    const Wavetable& WavetableBank::getTable (int index) const noexcept
    {
        jassert (! tables.empty());

        const auto clamped = juce::jlimit (0, static_cast<int> (tables.size()) - 1, index);
        return tables[static_cast<size_t> (clamped)];
    }
}
