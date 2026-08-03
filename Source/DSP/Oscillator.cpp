#include "DSP/Oscillator.h"

#include <cmath>

namespace nog::dsp
{
    namespace
    {
        constexpr float maxDetuneCents    = 100.0f;   // per side, at detune = 1
        constexpr float centsToRatioScale = 1.0f / 1200.0f;

        float midiNoteToHz (float note) noexcept
        {
            return 440.0f * std::exp2 ((note - 69.0f) / 12.0f);
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
        {
            voice.phase = 0.0;
            voice.finished = false;
        }

        layoutDirty = true;
        updateUnisonLayout();
    }

    void Oscillator::noteOn() noexcept
    {
        updateUnisonLayout();

        const auto playingSample = isPlayingSample();

        for (int i = 0; i < activeUnisonVoices; ++i)
        {
            auto& voice = unison[static_cast<size_t> (i)];

            if (playingSample)
            {
                // A sample starts where the user put the start offset. Randomising
                // it the way a wavetable does would scatter the attack transient,
                // which is usually the most recognisable part of the file.
                voice.phase = juce::jlimit (0.0f, 0.999f, settings.morph);
            }
            else
            {
                const auto randomOffset = random.nextFloat() * settings.phaseRandom;
                voice.phase = std::fmod (static_cast<double> (settings.phase + randomOffset), 1.0);
            }

            voice.finished = false;
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

        const auto maxFrame = static_cast<float> (juce::jmax (0, frameCount - 1));
        const auto spreadFrames = juce::jlimit (0.0f, 1.0f, settings.tableSpread) * maxFrame * 0.5f;

        auto gainSum = 0.0f;

        for (int i = 0; i < count; ++i)
        {
            auto& voice = unison[static_cast<size_t> (i)];

            // Spread from -1 to +1 across the unison stack, 0 for a lone voice.
            const auto spread = centreOnly ? 0.0f
                                           : (2.0f * static_cast<float> (i) / static_cast<float> (count - 1)) - 1.0f;

            const auto cents = spread * settings.detune * maxDetuneCents;
            voice.detuneRatio = std::exp2 (cents * centsToRatioScale);

            // Fanned around the morph position, and clamped rather than wrapped:
            // the ends of a table are usually nothing like each other, so a voice
            // that wrapped round would jump to an unrelated timbre.
            voice.frame = juce::jlimit (0.0f, maxFrame, framePosition + spread * spreadFrames);

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

    double Oscillator::incrementFor (float voiceFrequency) const noexcept
    {
        if (isPlayingSample())
        {
            const auto length = sample->getLength();

            if (length <= 1)
                return 0.0;

            // How far off the sample's own root the note is, applied on top of
            // the rate conversion between the file and the host.
            const auto rootFrequency = midiNoteToHz (static_cast<float> (settings.rootNote));
            const auto pitchRatio = rootFrequency > 0.0f
                                  ? static_cast<double> (voiceFrequency / rootFrequency)
                                  : 1.0;

            return sample->getBaseRatio (sampleRate) * pitchRatio / static_cast<double> (length);
        }

        return static_cast<double> (voiceFrequency) / sampleRate;
    }

    float Oscillator::readSource (double phase, double increment, int channel, float frame) const noexcept
    {
        if (isPlayingSample())
            return sample->read (channel, phase);

        return readTable (phase, increment, frame);
    }

    float Oscillator::readTable (double phase, double increment, float frame) const noexcept
    {
        if (table == nullptr || table->isEmpty())
            return 0.0f;

        const auto mip = Wavetable::mipForIncrement (increment);

        return table->getSample (frame, mip, phase - std::floor (phase));
    }

    float Oscillator::warpedSample (double phase, double increment, int channel, float frame) const noexcept
    {
        const auto amount = juce::jlimit (0.0f, 1.0f, settings.warpAmount);

        if (amount <= 0.0f)
            return readSource (phase, increment, channel, frame);

        switch (static_cast<Warp> (settings.warpMode))
        {
            case Warp::Off:
                return readSource (phase, increment, channel, frame);

            case Warp::Sync:
            {
                // Runs the wave faster and restarts it at the base period,
                // producing the hard-sync formant sweep. The increment is
                // scaled too, so the mip level follows the real bandwidth.
                const auto ratio = 1.0 + static_cast<double> (amount) * 3.0;
                return readSource (std::fmod (phase * ratio, 1.0), increment * ratio, channel, frame);
            }

            case Warp::BendPlus:
            {
                const auto exponent = 1.0 + static_cast<double> (amount) * 3.0;
                return readSource (std::pow (phase, exponent), increment * exponent, channel, frame);
            }

            case Warp::BendMinus:
            {
                const auto exponent = 1.0 / (1.0 + static_cast<double> (amount) * 3.0);
                return readSource (std::pow (phase, exponent), increment / exponent, channel, frame);
            }

            case Warp::Pwm:
            {
                // Subtracting a phase-shifted copy turns any wave into a
                // pulse-width-modulated version of itself.
                const auto offset = 0.5 - static_cast<double> (amount) * 0.49;
                return 0.5f * (readSource (phase, increment, channel, frame)
                             - readSource (std::fmod (phase + offset, 1.0), increment, channel, frame));
            }

            case Warp::Mirror:
            {
                const auto folded = phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
                const auto mixed  = phase + (folded - phase) * static_cast<double> (amount);
                return readSource (mixed, increment * 2.0, channel, frame);
            }

            case Warp::Asymmetric:
            {
                // Squeezes the first half of the cycle and stretches the second.
                const auto pivot  = 0.5 - static_cast<double> (amount) * 0.45;
                const auto mapped = phase < pivot ? (phase / pivot) * 0.5
                                                  : 0.5 + ((phase - pivot) / (1.0 - pivot)) * 0.5;
                return readSource (mapped, increment / juce::jmax (0.05, pivot * 2.0), channel, frame);
            }

            case Warp::Quantize:
            {
                const auto steps   = juce::jmax (2.0, std::round (64.0 * (1.0 - static_cast<double> (amount)) + 2.0));
                const auto stepped = std::floor (phase * steps) / steps;
                return readSource (stepped, increment, channel, frame);
            }
        }

        return readSource (phase, increment, channel, frame);
    }

    void Oscillator::addNextSample (float& left, float& right) noexcept
    {
        updateUnisonLayout();

        const auto playingSample = isPlayingSample();

        if (settings.level <= 0.0f || frequency <= 0.0f)
            return;

        if (! playingSample && table == nullptr)
            return;

        // Equal-power pan for the oscillator as a whole, applied on top of the
        // per-unison-voice placement.
        const auto panAngle = (juce::jlimit (-1.0f, 1.0f, settings.pan) + 1.0f) * 0.25f
                            * juce::MathConstants<float>::pi;
        const auto panLeft  = std::cos (panAngle) * juce::MathConstants<float>::sqrt2;
        const auto panRight = std::sin (panAngle) * juce::MathConstants<float>::sqrt2;

        const auto gain = settings.level * unisonNormalise;

        // A stereo file is read twice, once per side; a mono one is read once
        // and shared, which is both correct and half the work.
        const auto stereoSource = playingSample && sample->getNumChannels() > 1;
        const auto looping = playingSample
                          && static_cast<Loop> (settings.loop) == Loop::Forward;
        const auto loopStart = static_cast<double> (juce::jlimit (0.0f, 0.999f, settings.morph));

        auto sumLeft  = 0.0f;
        auto sumRight = 0.0f;

        for (int i = 0; i < activeUnisonVoices; ++i)
        {
            auto& voice = unison[static_cast<size_t> (i)];

            if (voice.finished)
                continue;

            const auto voiceFrequency = frequency * voice.detuneRatio;
            const auto increment      = incrementFor (voiceFrequency);

            // Anything at or above Nyquist can only alias. Samples are exempt:
            // their increment is a fraction of the file, not of a cycle.
            if (! playingSample && increment >= 0.5)
                continue;

            const auto sampleLeft  = warpedSample (voice.phase, increment, 0, voice.frame);
            const auto sampleRight = stereoSource ? warpedSample (voice.phase, increment, 1, voice.frame)
                                                  : sampleLeft;

            sumLeft  += sampleLeft  * voice.gainLeft;
            sumRight += sampleRight * voice.gainRight;

            voice.phase += increment;

            if (voice.phase >= 1.0)
            {
                if (! playingSample)
                    voice.phase -= std::floor (voice.phase);
                else if (looping)
                    voice.phase = loopStart;
                else
                    voice.finished = true;   // one-shot has run out
            }
        }

        left  += sumLeft  * gain * panLeft;
        right += sumRight * gain * panRight;
    }
}
