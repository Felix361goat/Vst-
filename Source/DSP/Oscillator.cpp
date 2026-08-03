#include "DSP/Oscillator.h"

#include <cmath>

namespace nog::dsp
{
    namespace
    {
        constexpr int   noiseTableSize    = 2048;
        constexpr float maxDetuneCents    = 100.0f;   // per side, at detune = 1
        constexpr float centsToRatioScale = 1.0f / 1200.0f;

        /** A fixed pseudo-random table, built once and shared.

            Seeded deterministically so that the "noise table" wave is identical
            in every instance and on every run - a patch has to sound the same
            when the project is reopened.
        */
        const std::array<float, noiseTableSize>& noiseTable()
        {
            static const auto table = []
            {
                std::array<float, noiseTableSize> t {};
                juce::Random seeded (0x9E3779B9);

                for (auto& sample : t)
                    sample = seeded.nextFloat() * 2.0f - 1.0f;

                return t;
            }();

            return table;
        }
    }

    void Oscillator::prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
    }

    void Oscillator::reset() noexcept
    {
        for (auto& voice : unison)
            voice.phase = 0.0;

        updateUnisonLayout();
    }

    void Oscillator::noteOn() noexcept
    {
        updateUnisonLayout();

        for (int i = 0; i < activeUnisonVoices; ++i)
        {
            const auto randomOffset = random.nextFloat() * settings.phaseRandom;
            unison[static_cast<size_t> (i)].phase =
                std::fmod (static_cast<double> (settings.phase + randomOffset), 1.0);
        }
    }

    void Oscillator::updateUnisonLayout() noexcept
    {
        activeUnisonVoices = juce::jlimit (1, maxUnison, settings.unisonVoices);

        const auto count      = activeUnisonVoices;
        const auto centreOnly = count == 1;
        const auto width      = juce::jlimit (0.0f, 1.0f, settings.width);
        const auto blend      = juce::jlimit (0.0f, 1.0f, settings.blend);

        auto gainSum = 0.0f;

        for (int i = 0; i < count; ++i)
        {
            auto& voice = unison[static_cast<size_t> (i)];

            // Spread from -1 to +1 across the unison stack, 0 for a lone voice.
            const auto spread = centreOnly ? 0.0f
                                           : (2.0f * static_cast<float> (i) / static_cast<float> (count - 1)) - 1.0f;

            const auto cents = spread * settings.detune * maxDetuneCents;
            voice.detuneRatio = std::exp2 (cents * centsToRatioScale);

            // Blend fades between the centre of the stack and its edges, which
            // is what turns a unison stack from "thicker" into "wider".
            const auto distanceFromCentre = std::abs (spread);
            const auto gain = centreOnly ? 1.0f
                                         : juce::jmax (0.0f, (1.0f - blend) * (1.0f - distanceFromCentre)
                                                                 + blend * distanceFromCentre);

            // Equal-power pan of this unison voice within the stereo field.
            const auto panPosition = juce::jlimit (-1.0f, 1.0f, spread * width);
            const auto panAngle    = (panPosition + 1.0f) * 0.25f * juce::MathConstants<float>::pi;

            voice.gainLeft  = gain * std::cos (panAngle);
            voice.gainRight = gain * std::sin (panAngle);

            gainSum += gain;
        }

        // Normalise so that adding unison voices does not add loudness. The
        // square root keeps the stack from sounding thin, since detuned voices
        // sum incoherently rather than linearly.
        unisonNormalise = gainSum > 0.0f ? 1.0f / std::sqrt (gainSum) : 1.0f;
    }

    float Oscillator::polyBlep (double t, double dt) noexcept
    {
        // Smooths the one-sample neighbourhood of a waveform discontinuity,
        // which removes most of the aliasing a naive saw or square produces.
        if (dt <= 0.0)
            return 0.0f;

        if (t < dt)
        {
            const auto x = t / dt;
            return static_cast<float> (x + x - x * x - 1.0);
        }

        if (t > 1.0 - dt)
        {
            const auto x = (t - 1.0) / dt;
            return static_cast<float> (x * x + x + x + 1.0);
        }

        return 0.0f;
    }

    float Oscillator::noiseTableSample (double phase) noexcept
    {
        const auto& table    = noiseTable();
        const auto  position = phase * noiseTableSize;
        const auto  index    = static_cast<int> (position) % noiseTableSize;
        const auto  next     = (index + 1) % noiseTableSize;
        const auto  fraction = static_cast<float> (position - std::floor (position));

        return table[static_cast<size_t> (index)]
             + (table[static_cast<size_t> (next)] - table[static_cast<size_t> (index)]) * fraction;
    }

    float Oscillator::sampleForWave (int waveIndex, double phase, double increment) const noexcept
    {
        switch (static_cast<Wave> (waveIndex))
        {
            case Wave::Sine:
                return std::sin (static_cast<float> (phase) * juce::MathConstants<float>::twoPi);

            case Wave::Triangle:
                // Harmonics fall off as 1/n^2, so naive generation is clean
                // enough here without a band-limiting correction.
                return 4.0f * std::abs (static_cast<float> (phase) - 0.5f) - 1.0f;

            case Wave::Saw:
                return static_cast<float> (2.0 * phase - 1.0) - polyBlep (phase, increment);

            case Wave::Square:
            {
                const auto naive = phase < 0.5 ? 1.0f : -1.0f;
                return naive - polyBlep (phase, increment)
                             + polyBlep (std::fmod (phase + 0.5, 1.0), increment);
            }

            case Wave::Pulse:
            {
                constexpr double dutyCycle = 0.25;
                const auto naive = phase < dutyCycle ? 1.0f : -1.0f;
                return naive - polyBlep (phase, increment)
                             + polyBlep (std::fmod (phase + (1.0 - dutyCycle), 1.0), increment);
            }

            case Wave::NoiseTable:
                return noiseTableSample (phase);
        }

        return 0.0f;
    }

    float Oscillator::morphedSample (double phase, double increment) const noexcept
    {
        // The wave selector picks the starting point and morph travels forwards
        // through the bank from there, wrapping at the end. With real
        // wavetables this becomes a frame index into the loaded table.
        const auto position = static_cast<float> (settings.wave)
                            + juce::jlimit (0.0f, 1.0f, settings.morph) * static_cast<float> (numWaves - 1);

        const auto lower    = static_cast<int> (std::floor (position));
        const auto fraction = position - static_cast<float> (lower);

        const auto lowerIndex = ((lower % numWaves) + numWaves) % numWaves;
        const auto upperIndex = (lowerIndex + 1) % numWaves;

        if (fraction < 1.0e-4f)
            return sampleForWave (lowerIndex, phase, increment);

        const auto a = sampleForWave (lowerIndex, phase, increment);
        const auto b = sampleForWave (upperIndex, phase, increment);

        return a + (b - a) * fraction;
    }

    float Oscillator::warpedSample (double phase, double increment) const noexcept
    {
        const auto amount = juce::jlimit (0.0f, 1.0f, settings.warpAmount);

        if (amount <= 0.0f)
            return morphedSample (phase, increment);

        switch (static_cast<Warp> (settings.warpMode))
        {
            case Warp::Off:
                return morphedSample (phase, increment);

            case Warp::Sync:
            {
                // Runs the wave faster and restarts it at the base period,
                // producing the hard-sync formant sweep.
                const auto ratio   = 1.0 + static_cast<double> (amount) * 3.0;
                const auto synced  = std::fmod (phase * ratio, 1.0);
                return morphedSample (synced, increment * ratio);
            }

            case Warp::BendPlus:
            {
                const auto exponent = 1.0 + static_cast<double> (amount) * 3.0;
                return morphedSample (std::pow (phase, exponent), increment);
            }

            case Warp::BendMinus:
            {
                const auto exponent = 1.0 / (1.0 + static_cast<double> (amount) * 3.0);
                return morphedSample (std::pow (phase, exponent), increment);
            }

            case Warp::Pwm:
            {
                // Subtracting a phase-shifted copy turns any wave into a
                // pulse-width-modulated version of itself.
                const auto offset = 0.5 - static_cast<double> (amount) * 0.49;
                return 0.5f * (morphedSample (phase, increment)
                             - morphedSample (std::fmod (phase + offset, 1.0), increment));
            }

            case Warp::Mirror:
            {
                const auto folded = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
                const auto mixed  = phase + (folded - phase) * static_cast<double> (amount);
                return morphedSample (mixed, increment);
            }

            case Warp::Asymmetric:
            {
                // Squeezes the first half of the cycle and stretches the second.
                const auto pivot = 0.5 - static_cast<double> (amount) * 0.45;
                const auto mapped = phase < pivot ? (phase / pivot) * 0.5
                                                  : 0.5 + ((phase - pivot) / (1.0 - pivot)) * 0.5;
                return morphedSample (mapped, increment);
            }

            case Warp::Quantize:
            {
                const auto steps = juce::jmax (2.0, std::round (64.0 * (1.0 - static_cast<double> (amount)) + 2.0));
                const auto stepped = std::floor (phase * steps) / steps;
                return morphedSample (stepped, increment);
            }
        }

        return morphedSample (phase, increment);
    }

    void Oscillator::addNextSample (float& left, float& right) noexcept
    {
        if (settings.level <= 0.0f || frequency <= 0.0f)
            return;

        updateUnisonLayout();

        // Equal-power pan for the oscillator as a whole, applied on top of the
        // per-unison-voice placement.
        const auto panAngle = (juce::jlimit (-1.0f, 1.0f, settings.pan) + 1.0f) * 0.25f
                            * juce::MathConstants<float>::pi;
        const auto panLeft  = std::cos (panAngle) * juce::MathConstants<float>::sqrt2;
        const auto panRight = std::sin (panAngle) * juce::MathConstants<float>::sqrt2;

        const auto gain = settings.level * unisonNormalise;

        auto sumLeft  = 0.0f;
        auto sumRight = 0.0f;

        for (int i = 0; i < activeUnisonVoices; ++i)
        {
            auto& voice = unison[static_cast<size_t> (i)];

            const auto voiceFrequency = frequency * voice.detuneRatio;
            const auto increment      = static_cast<double> (voiceFrequency) / sampleRate;

            // Anything at or above Nyquist can only alias, so it is dropped.
            if (increment >= 0.5)
                continue;

            const auto sample = warpedSample (voice.phase, increment);

            sumLeft  += sample * voice.gainLeft;
            sumRight += sample * voice.gainRight;

            voice.phase += increment;

            if (voice.phase >= 1.0)
                voice.phase -= std::floor (voice.phase);
        }

        left  += sumLeft  * gain * panLeft;
        right += sumRight * gain * panRight;
    }
}
