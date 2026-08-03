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
        soundingNote = -1;
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
            if (soundingNote >= 0)
            {
                events.push_back ({ 0, soundingNote, 0.0f, false });
                soundingNote = -1;
            }

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
            // Swing lengthens every other step and shortens the one after, which
            // is the whole of what makes a pattern shuffle.
            const auto swung = (stepIndex % 2) == 0
                             ? stepLength * (1.0 + static_cast<double> (settings.swing) * 0.33)
                             : stepLength * (1.0 - static_cast<double> (settings.swing) * 0.33);

            const auto remainingInStep = juce::jmax (1.0, swung - stepPhase);
            const auto chunk = juce::jmin (static_cast<double> (numSamples - consumed), remainingInStep);

            // Gate: release the note partway through its step.
            if (soundingNote >= 0 && gateRemaining > 0.0)
            {
                if (gateRemaining <= chunk)
                {
                    events.push_back ({ consumed + static_cast<int> (gateRemaining), soundingNote, 0.0f, false });
                    soundingNote = -1;
                    gateRemaining = 0.0;
                }
                else
                {
                    gateRemaining -= chunk;
                }
            }

            stepPhase += chunk;
            consumed += static_cast<int> (chunk);

            if (stepPhase + 0.5 >= swung)
            {
                stepPhase = 0.0;

                // Anything still sounding is cut before the next note starts.
                if (soundingNote >= 0)
                {
                    events.push_back ({ juce::jmin (consumed, numSamples - 1), soundingNote, 0.0f, false });
                    soundingNote = -1;
                }

                const auto mode = static_cast<Mode> (settings.mode);
                const auto offset = juce::jlimit (0, numSamples - 1, consumed);

                if (mode == Mode::Chord)
                {
                    for (const auto& note : pattern)
                        events.push_back ({ offset, note.note, note.velocity, true });

                    // Chord mode has no single sounding note to gate, so the
                    // notes are released by the next step's clear-down.
                    soundingNote = -1;
                }
                else
                {
                    const auto index = mode == Mode::Random
                                     ? random.nextInt (static_cast<int> (pattern.size()))
                                     : stepIndex % static_cast<int> (pattern.size());

                    const auto& note = pattern[static_cast<size_t> (index)];
                    events.push_back ({ offset, note.note, note.velocity, true });

                    soundingNote = note.note;
                    gateRemaining = swung * static_cast<double> (juce::jlimit (0.05f, 1.0f, settings.gate));
                }

                ++stepIndex;

                if (stepIndex >= 1000000)
                    stepIndex = 0;
            }
        }
    }
}
