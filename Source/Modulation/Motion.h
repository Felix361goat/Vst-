#pragma once

#include <array>

#include <juce_core/juce_core.h>

#include "Params/ParameterIDs.h"

namespace nog
{
    /**
        A tempo-locked step pattern, belonging to the instrument rather than to
        a note.

        Every LFO in the synth is per-voice, which is right for vibrato and
        wrong for anything downstream of the voices: the effects rack runs once
        on the summed output, and thirty-two voices each with their own opinion
        about the filter cutoff of a delay is not a sensible thing to build.
        Motion exists to drive those.

        It is a step sequencer rather than a waveform because that is what makes
        a part sound sequenced instead of merely wobbling. A gate pattern on the
        effect mix, a filter that moves in sixteenths, a pan that hops - none of
        those are a sine wave, and reaching for them with one is the difference
        between a plugin that adds rhythm and one that adds seasickness.

        The clock is derived from the host's playhead position rather than
        accumulated locally, so it stays locked to the bar however the transport
        is scrubbed, looped or restarted. When the host is not playing it free
        runs, so the pattern is still audible while auditioning a patch.
    */
    class Motion
    {
    public:
        struct Settings
        {
            bool  enabled  = false;
            int   division = 7;      // index into tempoDivisions(); 7 is a sixteenth
            float smooth   = 0.0f;   // 0 stepped, 1 fully rounded off
            float swing    = 0.0f;   // 0 straight, 1 heavily swung
            float depth    = 1.0f;   // scales the whole pattern

            std::array<float, ids::numMotionSteps> steps {};
        };

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            reset();
        }

        void reset() noexcept
        {
            freeRunPosition = 0.0;
            smoothed = 0.0f;
            value = 0.0f;
        }

        /** Advances by @p numSamples and returns the pattern's current level,
            0..1. @p positionInBeats is the host's playhead, or a negative
            number when the transport is not running. */
        float advance (int numSamples, double bpm, double positionInBeats,
                       const Settings& settings, double beatsPerStep) noexcept
        {
            if (! settings.enabled || beatsPerStep <= 0.0)
            {
                value = 0.0f;
                smoothed = 0.0f;
                return 0.0f;
            }

            const auto secondsPerBeat = 60.0 / juce::jmax (1.0, bpm);
            const auto beats = positionInBeats >= 0.0
                             ? positionInBeats
                             : freeRunPosition;

            // The pattern repeats over its whole length, so the phase is taken
            // against the pattern rather than against a single step - that is
            // what keeps step one on the downbeat.
            const auto patternBeats = beatsPerStep * ids::numMotionSteps;
            const auto phase = std::fmod (beats / patternBeats, 1.0);
            const auto positionInPattern = (phase < 0.0 ? phase + 1.0 : phase) * ids::numMotionSteps;

            const auto target = readPattern (positionInPattern, settings);

            // Smoothing is a one-pole over the step edges. Its time constant is
            // a fraction of a step rather than a fixed number of milliseconds,
            // so a pattern sounds the same shape at any tempo.
            const auto stepSamples = juce::jmax (1.0, beatsPerStep * secondsPerBeat * sampleRate);
            const auto coefficient = settings.smooth <= 0.0f
                                   ? 1.0f
                                   : 1.0f - std::exp (-static_cast<float> (numSamples)
                                                      / static_cast<float> (stepSamples * settings.smooth * 0.5));

            smoothed += coefficient * (target - smoothed);

            freeRunPosition += numSamples / (secondsPerBeat * sampleRate);

            value = juce::jlimit (0.0f, 1.0f, smoothed * settings.depth);
            return value;
        }

        float getValue() const noexcept { return value; }

    private:
        /** The step level at a fractional position through the pattern, with
            swing applied by moving the step boundaries rather than the clock. */
        static float readPattern (double position, const Settings& settings) noexcept
        {
            const auto count = ids::numMotionSteps;
            auto index = static_cast<int> (position) % count;

            if (index < 0)
                index += count;

            if (settings.swing > 0.0f)
            {
                // Every second step starts late by up to a third of a step,
                // which is the amount that reads as swing rather than as error.
                const auto within = position - std::floor (position);
                const auto shift  = static_cast<double> (settings.swing) * 0.33;

                if (index % 2 == 1 && within < shift)
                    index = index == 0 ? count - 1 : index - 1;
                else if (index % 2 == 0 && within > 1.0 - shift)
                    index = (index + 1) % count;
            }

            return juce::jlimit (0.0f, 1.0f, settings.steps[static_cast<size_t> (index)]);
        }

        double sampleRate = 44100.0;
        double freeRunPosition = 0.0;
        float  smoothed = 0.0f;
        float  value = 0.0f;
    };
}
