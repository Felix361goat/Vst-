#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        A per-voice low frequency oscillator.

        Rate is always supplied in Hz; tempo synchronisation is resolved further
        up, where the host's BPM is known, so this class stays free of transport
        concerns and is trivial to test.

        Rise gives the classic delayed-vibrato behaviour by fading the output in
        over a fixed time after a retrigger, and Smooth one-poles the output so
        that stepped shapes (square, sample & hold) can be softened into
        something usable on a pitch or cutoff destination.
    */
    class Lfo
    {
    public:
        enum class Shape { Sine = 0, Triangle, SawUp, SawDown, Square, SampleAndHold, RandomGlide };
        enum class TriggerMode { Trigger = 0, Envelope, FreeRun };

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            reset();
        }

        void reset() noexcept
        {
            phase          = 0.0;
            riseProgress   = 0.0f;
            smoothedValue  = 0.0f;
            currentValue   = 0.0f;
            heldValue      = nextHeldValue();
            glideFrom      = heldValue;
            glideTo        = nextHeldValue();
        }

        void setShape (Shape s) noexcept          { shape = s; }
        void setRateHz (float hz) noexcept        { rateHz = juce::jmax (0.0f, hz); }
        void setPhaseOffset (float p) noexcept    { phaseOffset = p; }
        void setBipolar (bool b) noexcept         { bipolar = b; }
        void setRiseMs (float ms) noexcept        { riseMs = juce::jmax (0.0f, ms); }

        /** 0 = no smoothing, 1 = heavily smoothed. */
        void setSmoothing (float amount) noexcept { smoothing = juce::jlimit (0.0f, 1.0f, amount); }

        /** Restarts the cycle. Ignored in free-run mode, where the LFO is meant
            to keep a continuous phase across notes. */
        void retrigger (TriggerMode mode) noexcept
        {
            if (mode == TriggerMode::FreeRun)
                return;

            phase        = 0.0;
            riseProgress = 0.0f;
        }

        float getValue() const noexcept { return currentValue; }

        /** Advances the LFO by @p numSamples and returns the new value.

            As with the envelopes, voices run this at control rate, so the step
            count is explicit. Passing 1 gives per-sample behaviour.
        */
        float getNextValue (int numSamples = 1) noexcept
        {
            const auto steps     = static_cast<double> (juce::jmax (1, numSamples));
            const auto increment = static_cast<double> (rateHz) * steps / sampleRate;

            auto raw = evaluate (std::fmod (phase + static_cast<double> (phaseOffset), 1.0));

            phase += increment;

            if (phase >= 1.0)
            {
                phase -= std::floor (phase);
                onCycleWrapped();
            }

            // One-pole smoothing. The coefficient is shaped so the knob's upper
            // end is genuinely slow rather than everything happening at the top.
            if (smoothing > 0.0f)
            {
                const auto coefficient = 1.0f - std::pow (1.0f - smoothing, 3.0f) * 0.999f;
                smoothedValue += (raw - smoothedValue) * (1.0f - coefficient);
                raw = smoothedValue;
            }
            else
            {
                smoothedValue = raw;
            }

            if (riseMs > 0.0f && riseProgress < 1.0f)
            {
                riseProgress += static_cast<float> (steps)
                              / juce::jmax (1.0f, static_cast<float> (sampleRate) * riseMs * 0.001f);
                riseProgress  = juce::jmin (1.0f, riseProgress);
                raw          *= riseProgress;
            }

            currentValue = bipolar ? raw : (raw + 1.0f) * 0.5f;
            return currentValue;
        }

    private:
        /** Shapes are generated over -1..1; the unipolar conversion happens once
            at the end of getNextValue. */
        float evaluate (double p) const noexcept
        {
            const auto x = static_cast<float> (p);

            switch (shape)
            {
                case Shape::Sine:      return std::sin (x * juce::MathConstants<float>::twoPi);
                case Shape::Triangle:  return 4.0f * std::abs (x - 0.5f) - 1.0f;
                case Shape::SawUp:     return 2.0f * x - 1.0f;
                case Shape::SawDown:   return 1.0f - 2.0f * x;
                case Shape::Square:    return x < 0.5f ? 1.0f : -1.0f;
                case Shape::SampleAndHold: return heldValue;
                case Shape::RandomGlide:   return glideFrom + (glideTo - glideFrom) * x;
            }

            return 0.0f;
        }

        void onCycleWrapped() noexcept
        {
            heldValue = nextHeldValue();
            glideFrom = glideTo;
            glideTo   = nextHeldValue();
        }

        float nextHeldValue() noexcept { return random.nextFloat() * 2.0f - 1.0f; }

        double sampleRate    = 44100.0;
        double phase         = 0.0;
        float  phaseOffset   = 0.0f;
        float  rateHz        = 2.0f;
        float  riseMs        = 0.0f;
        float  riseProgress  = 1.0f;
        float  smoothing     = 0.0f;
        float  smoothedValue = 0.0f;
        float  currentValue  = 0.0f;
        float  heldValue     = 0.0f;
        float  glideFrom     = 0.0f;
        float  glideTo       = 0.0f;
        bool   bipolar       = true;

        Shape  shape = Shape::Sine;
        juce::Random random;
    };
}
