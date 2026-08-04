#include "Engine/Arpeggiator.h"

#include "Params/ParameterLayout.h"

namespace nog
{
    void Arpeggiator::prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
    }

    void Arpeggiator::reset()
    {
        held.clear();
        pattern.clear();
        stepPhase = 0.0;
        stepIndex = 0;
        sounding.clear();
        gateRemaining = 0.0;
        descending = false;
    }

    void Arpeggiator::noteOn (int note, float velocity)
    {
        for (auto& entry : held)
        {
            if (entry.note == note)
            {
                entry.velocity = velocity;
                return;
            }
        }

        const auto wasEmpty = held.empty();
        held.push_back ({ note, velocity });

        // Starting from silence restarts the pattern, so the first note lands
        // immediately rather than wherever the free-running clock happened to be.
        if (wasEmpty)
        {
            stepPhase = 0.0;
            stepIndex = 0;
            descending = false;
        }
    }

    void Arpeggiator::noteOff (int note)
    {
        for (auto entry = held.begin(); entry != held.end(); ++entry)
        {
            if (entry->note == note)
            {
                held.erase (entry);
                return;
            }
        }
    }

    double Arpeggiator::stepLengthInSamples (double bpm, const Settings& settings) const noexcept
    {
        const auto beats = params::choices::tempoDivisionInBeats (settings.division);
        const auto safeBpm = bpm > 1.0 ? bpm : 120.0;

        return juce::jmax (16.0, 60.0 / safeBpm * beats * sampleRate);
    }

    int Arpeggiator::samplesUntilNextStep (double bpm, const Settings& settings) const noexcept
    {
        if (! settings.enabled || held.empty())
            return std::numeric_limits<int>::max();

        const auto length = stepLengthInSamples (bpm, settings);
        return juce::jmax (1, static_cast<int> (std::ceil (length - stepPhase)));
    }

    void Arpeggiator::rebuildPattern (const Settings& settings)
    {
        pattern.clear();

        if (held.empty())
            return;

        auto sorted = held;
        std::sort (sorted.begin(), sorted.end(),
                   [] (const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

        const auto mode = static_cast<Mode> (settings.mode);
        const auto octaves = juce::jlimit (1, 4, settings.octaves);

        // Chord mode plays everything at once, so the pattern is one step long
        // and the engine reads the whole held list instead.
        if (mode == Mode::Chord)
        {
            pattern = sorted;
            return;
        }

        const auto& base = mode == Mode::AsPlayed ? held : sorted;

        for (int octave = 0; octave < octaves; ++octave)
            for (const auto& note : base)
                pattern.push_back ({ note.note + octave * 12, note.velocity });

        if (mode == Mode::Down)
        {
            std::reverse (pattern.begin(), pattern.end());
        }
        else if (mode == Mode::UpDown || mode == Mode::DownUp)
        {
            // The turning notes are not repeated, which is what keeps an
            // up-down pattern feeling even rather than limping at the ends.
            auto reversed = pattern;
            std::reverse (reversed.begin(), reversed.end());

            if (reversed.size() > 2)
            {
                reversed.erase (reversed.begin());
                reversed.pop_back();
            }
            else
            {
                reversed.clear();
            }

            if (mode == Mode::DownUp)
            {
                std::reverse (pattern.begin(), pattern.end());
                std::reverse (reversed.begin(), reversed.end());
            }

            pattern.insert (pattern.end(), reversed.begin(), reversed.end());
        }
    }

    void Arpeggiator::process (int numSamples, double bpm, const Settings& settings,
                               std::vector<Event>& events)
    {
        if (numSamples <= 0)
            return;

        // Turning the arp off, or letting go of every key, has to release
        // whatever it left sounding.
        if (! settings.enabled || held.empty())
        {
            for (const auto note : sounding)
                events.push_back ({ 0, note, 0.0f, false });

            sounding.clear();

            stepPhase = 0.0;
            stepIndex = 0;
            gateRemaining = 0.0;
            return;
        }

        rebuildPattern (settings);

        if (pattern.empty())
            return;

        const auto stepLength = stepLengthInSamples (bpm, settings);
        auto consumed = 0;

        while (consumed < numSamples)
        {
            // A step fires at its *start*, not its end. Firing at the end meant
            // the first note of a pattern arrived one whole step after the key
            // went down - inaudible at a sixteenth, and half a beat of silence
            // at a quarter, which is long enough to feel broken.
            if (stepPhase <= 0.0)
            {
                // Swing lengthens every other step and shortens the one after,
                // which is the whole of what makes a pattern shuffle. Held for
                // the length of the step rather than recomputed, because the
                // step index moves on as soon as the note has fired.
                currentStepLength = (stepIndex % 2) == 0
                                  ? stepLength * (1.0 + static_cast<double> (settings.swing) * 0.33)
                                  : stepLength * (1.0 - static_cast<double> (settings.swing) * 0.33);

                // Anything still sounding is cut before the next note starts.
                for (const auto note : sounding)
                    events.push_back ({ juce::jmin (consumed, numSamples - 1), note, 0.0f, false });

                sounding.clear();

                const auto mode = static_cast<Mode> (settings.mode);
                const auto offset = juce::jlimit (0, numSamples - 1, consumed);

                if (mode == Mode::Chord)
                {
                    for (const auto& note : pattern)
                    {
                        events.push_back ({ offset, note.note, note.velocity, true });
                        sounding.push_back (note.note);
                    }

                    // The whole chord is gated together, exactly as a single
                    // note would be. Leaving it ungated was what let a chord
                    // pattern hold voices open for ever.
                    gateRemaining = currentStepLength
                                  * static_cast<double> (juce::jlimit (0.05f, 1.0f, settings.gate));
                }
                else
                {
                    const auto index = mode == Mode::Random
                                     ? random.nextInt (static_cast<int> (pattern.size()))
                                     : stepIndex % static_cast<int> (pattern.size());

                    const auto& note = pattern[static_cast<size_t> (index)];
                    events.push_back ({ offset, note.note, note.velocity, true });

                    sounding.push_back (note.note);
                    gateRemaining = currentStepLength
                                  * static_cast<double> (juce::jlimit (0.05f, 1.0f, settings.gate));
                }

                ++stepIndex;

                if (stepIndex >= 1000000)
                    stepIndex = 0;
            }

            const auto remainingInStep = juce::jmax (1.0, currentStepLength - stepPhase);
            const auto chunk = juce::jmin (static_cast<double> (numSamples - consumed), remainingInStep);

            // Gate: release the note partway through its step.
            if (! sounding.empty() && gateRemaining > 0.0)
            {
                if (gateRemaining <= chunk)
                {
                    const auto offset = juce::jlimit (0, numSamples - 1,
                                                      consumed + static_cast<int> (gateRemaining));

                    for (const auto note : sounding)
                        events.push_back ({ offset, note, 0.0f, false });

                    sounding.clear();
                    gateRemaining = 0.0;
                }
                else
                {
                    gateRemaining -= chunk;
                }
            }

            stepPhase += chunk;
            consumed += static_cast<int> (chunk);

            if (stepPhase + 0.5 >= currentStepLength)
                stepPhase = 0.0;
        }
    }
}
