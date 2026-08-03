#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        An AHDSR envelope with per-stage curve control.

        Stage timing is driven by a normalised 0..1 ramp so that changing a
        stage's length mid-flight stretches the remaining time rather than
        jumping the output, which is what makes the envelope safe to modulate.

        The curve controls bend each stage between logarithmic (-1), linear (0)
        and exponential (+1) without changing its duration.
    */
    class Envelope
    {
    public:
        struct Settings
        {
            float attackMs     = 2.0f;
            float holdMs       = 0.0f;
            float decayMs      = 600.0f;
            float sustain      = 1.0f;
            float releaseMs    = 120.0f;
            float attackCurve  = 0.0f;
            float decayCurve   = 0.0f;
            float releaseCurve = 0.0f;
        };

        enum class Stage { Idle, Attack, Hold, Decay, Sustain, Release };

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            reset();
        }

        void setSettings (const Settings& s) noexcept { settings = s; }

        void noteOn() noexcept
        {
            stage      = Stage::Attack;
            stagePhase = 0.0f;
            // Starting the attack from the current value rather than zero keeps
            // a retriggered voice from clicking.
            stageStart = value;
        }

        void noteOff() noexcept
        {
            if (stage == Stage::Idle || stage == Stage::Release)
                return;

            stage      = Stage::Release;
            stagePhase = 0.0f;
            stageStart = value;
        }

        /** Silences the envelope immediately, without a release stage. */
        void reset() noexcept
        {
            stage      = Stage::Idle;
            stagePhase = 0.0f;
            stageStart = 0.0f;
            value      = 0.0f;
        }

        bool isActive() const noexcept  { return stage != Stage::Idle; }
        bool isReleasing() const noexcept { return stage == Stage::Release; }
        float getValue() const noexcept { return value; }
        Stage getStage() const noexcept { return stage; }

        /** Advances the envelope by @p numSamples and returns the new value.

            Voices run their modulation at control rate - once per short
            sub-block rather than once per sample - so the step count is
            explicit. Passing 1 gives ordinary per-sample behaviour.
        */
        float getNextValue (int numSamples = 1) noexcept
        {
            steps = juce::jmax (1, numSamples);

            switch (stage)
            {
                case Stage::Idle:
                    value = 0.0f;
                    break;

                // Each stage computes its output from the ramp *before* handing
                // over, so the sample on which a stage ends is its end value
                // rather than the next stage's starting value.
                case Stage::Attack:
                {
                    const auto finished = advance (settings.attackMs);
                    value = stageStart + (1.0f - stageStart) * shape (stagePhase, settings.attackCurve);

                    if (finished)
                    {
                        stage      = Stage::Hold;
                        stagePhase = 0.0f;
                        stageStart = 1.0f;
                    }

                    break;
                }

                case Stage::Hold:
                {
                    const auto finished = advance (settings.holdMs);
                    value = 1.0f;

                    if (finished)
                    {
                        stage      = Stage::Decay;
                        stagePhase = 0.0f;
                        stageStart = 1.0f;
                    }

                    break;
                }

                case Stage::Decay:
                {
                    const auto finished = advance (settings.decayMs);
                    value = stageStart + (settings.sustain - stageStart) * shape (stagePhase, settings.decayCurve);

                    if (finished)
                    {
                        stage      = Stage::Sustain;
                        stagePhase = 0.0f;
                    }

                    break;
                }

                case Stage::Sustain:
                    value = settings.sustain;
                    break;

                case Stage::Release:
                {
                    const auto finished = advance (settings.releaseMs);
                    value = stageStart * (1.0f - shape (stagePhase, settings.releaseCurve));

                    if (finished)
                        reset();

                    break;
                }
            }

            return value;
        }

    private:
        /** Advances the stage ramp; returns true once the stage has completed. */
        bool advance (float stageLengthMs) noexcept
        {
            const auto lengthInSamples = static_cast<float> (sampleRate) * stageLengthMs * 0.001f;

            // A stage shorter than the current step completes immediately rather
            // than dividing by zero - the common case for an attack of 0 ms.
            if (lengthInSamples < static_cast<float> (steps))
            {
                stagePhase = 1.0f;
                return true;
            }

            stagePhase += static_cast<float> (steps) / lengthInSamples;

            if (stagePhase >= 1.0f)
            {
                stagePhase = 1.0f;
                return true;
            }

            return false;
        }

        /** Bends a 0..1 ramp. curve < 0 is slow-then-fast, curve > 0 the
            reverse; 0 passes the ramp through untouched. */
        static float shape (float x, float curve) noexcept
        {
            if (std::abs (curve) < 1.0e-4f)
                return x;

            const auto exponent = std::pow (4.0f, -curve);
            return std::pow (juce::jlimit (0.0f, 1.0f, x), exponent);
        }

        double   sampleRate = 44100.0;
        Settings settings;
        Stage    stage      = Stage::Idle;
        float    stagePhase = 0.0f;
        float    stageStart = 0.0f;
        float    value      = 0.0f;
        int      steps      = 1;
    };
}
