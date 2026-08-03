#include "DSP/Oscillator.h"

#include <cmath>

namespace nog::dsp
{
    namespace
    {
        constexpr float maxDetuneCents    = 100.0f;   // per side, at detune = 1
        constexpr float centsToRatioScale = 1.0f / 1200.0f;
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

        layoutDirty = true;
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
        if (! layoutDirty)
            return;

        layoutDirty = false;

        activeUnisonVoices = juce::jlimit (1, maxUnison, settings.unisonVoices);

        const auto count      = activeUnisonVoices;
        const auto centreOnly = count == 1;
        const auto width      = juce::jlimit (0.0f, 1.0f, settings.width);
        const auto blend      = juce::jlimit (0.0f, 1.0f, settings.blend);

        // Frame index the morph control lands on, resolved once per update.
        const auto frameCount = table != nullptr ? table->getNumFrames() : 1;
        framePosition = juce::jlimit (0.0f, 1.0f, settings.morph)
                      * static_cast<float> (juce::jmax (0, frameCount - 1));

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

    float Oscillator::readTable (double phase, double increment) const noexcept
    {
        if (table == nullptr || table->isEmpty())
            return 0.0f;

        const auto mip = Wavetable::mipForIncrement (increment);

        return table->getSample (framePosition, mip, phase - std::floor (phase));
    }

    float Oscillator::warpedSample (double phase, double increment) const noexcept
    {
        const auto amount = juce::jlimit (0.0f, 1.0f, settings.warpAmount);

        if (amount <= 0.0f)
            return readTable (phase, increment);

        switch (static_cast<Warp> (settings.warpMode))
        {
            case Warp::Off:
                return readTable (phase, increment);

            case Warp::Sync:
            {
                // Runs the wave faster and restarts it at the base period,
                // producing the hard-sync formant sweep. The increment is
                // scaled too, so the mip level follows the real bandwidth.
                const auto ratio = 1.0 + static_cast<double> (amount) * 3.0;
                return readTable (std::fmod (phase * ratio, 1.0), increment * ratio);
            }

            case Warp::BendPlus:
            {
                const auto exponent = 1.0 + static_cast<double> (amount) * 3.0;
                return readTable (std::pow (phase, exponent), increment * exponent);
            }

            case Warp::BendMinus:
            {
                const auto exponent = 1.0 / (1.0 + static_cast<double> (amount) * 3.0);
                return readTable (std::pow (phase, exponent), increment / exponent);
            }

            case Warp::Pwm:
            {
                // Subtracting a phase-shifted copy turns any wave into a
                // pulse-width-modulated version of itself.
                const auto offset = 0.5 - static_cast<double> (amount) * 0.49;
                return 0.5f * (readTable (phase, increment)
                             - readTable (std::fmod (phase + offset, 1.0), increment));
            }

            case Warp::Mirror:
            {
                const auto folded = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
                const auto mixed  = phase + (folded - phase) * static_cast<double> (amount);
                return readTable (mixed, increment * 2.0);
            }

            case Warp::Asymmetric:
            {
                // Squeezes the first half of the cycle and stretches the second.
                const auto pivot  = 0.5 - static_cast<double> (amount) * 0.45;
                const auto mapped = phase < pivot ? (phase / pivot) * 0.5
                                                  : 0.5 + ((phase - pivot) / (1.0 - pivot)) * 0.5;
                return readTable (mapped, increment / juce::jmax (0.05, pivot * 2.0));
            }

            case Warp::Quantize:
            {
                const auto steps   = juce::jmax (2.0, std::round (64.0 * (1.0 - static_cast<double> (amount)) + 2.0));
                const auto stepped = std::floor (phase * steps) / steps;
                return readTable (stepped, increment);
            }
        }

        return readTable (phase, increment);
    }

    void Oscillator::addNextSample (float& left, float& right) noexcept
    {
        updateUnisonLayout();

        if (settings.level <= 0.0f || frequency <= 0.0f || table == nullptr)
            return;

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
