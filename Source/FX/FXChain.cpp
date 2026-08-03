#include "FX/FXChain.h"

namespace nog::fx
{
    namespace
    {
        constexpr float maxDelaySeconds = 2.0f;

        /** Bypass keeps a slot in the rack without touching the audio, so that
            an empty slot costs nothing and needs no special-casing. */
        class BypassEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec&) override {}
            void reset() override {}
            void setParameters (float, float, float) override {}
            void process (juce::dsp::AudioBlock<float>&) override {}

            std::array<const char*, 3> getControlNames() const override
            {
                return { "-", "-", "-" };
            }
        };

        /** A: drive, B: tone (low-pass), C: output trim. */
        class DistortionEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                sampleRate = spec.sampleRate;
                filter.prepare (spec);
                reset();
            }

            void reset() override { filter.reset(); }

            void setParameters (float a, float b, float c) override
            {
                drive  = 1.0f + a * 48.0f;
                output = c * 2.0f;

                // Make-up gain keeps the level roughly constant as drive rises,
                // so the knob changes character rather than loudness.
                makeUp = 1.0f / std::sqrt (drive);

                const auto cutoff = juce::jmap (b, 400.0f, juce::jmin (18000.0f, static_cast<float> (sampleRate) * 0.45f));
                *filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, cutoff);
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
                {
                    auto* samples = block.getChannelPointer (channel);

                    for (size_t i = 0; i < block.getNumSamples(); ++i)
                        samples[i] = std::tanh (samples[i] * drive) * makeUp * output;
                }

                juce::dsp::ProcessContextReplacing<float> context (block);
                filter.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Drive", "Tone", "Output" };
            }

        private:
            using Duplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                              juce::dsp::IIR::Coefficients<float>>;

            double     sampleRate = 44100.0;
            float      drive  = 1.0f;
            float      makeUp = 1.0f;
            float      output = 1.0f;
            Duplicator filter;
        };

        /** A: bit depth, B: sample rate, C: output.

            Two separate kinds of digital damage: quantising the amplitude, and
            holding each sample for several output samples. Both are deliberately
            un-smoothed, because the aliasing is the point.
        */
        class BitCrusherEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                sampleRate = spec.sampleRate;
                held.assign (spec.numChannels, 0.0f);
                reset();
            }

            void reset() override
            {
                std::fill (held.begin(), held.end(), 0.0f);
                holdCounter = 0.0f;
            }

            void setParameters (float a, float b, float c) override
            {
                // 16 bits down to 1. Inverted so the knob opens up into damage.
                const auto bits = juce::jmap (1.0f - a, 1.0f, 16.0f);
                levels = std::pow (2.0f, bits) - 1.0f;

                // Hold each sample for this many output samples.
                const auto targetRate = juce::jmap (1.0f - b,
                                                    static_cast<float> (sampleRate),
                                                    500.0f);
                step = juce::jmax (1.0f, static_cast<float> (sampleRate) / juce::jmax (1.0f, targetRate));

                output = c * 2.0f;
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                const auto numChannels = block.getNumChannels();
                const auto numSamples  = block.getNumSamples();

                if (held.size() < numChannels)
                    held.assign (numChannels, 0.0f);

                for (size_t i = 0; i < numSamples; ++i)
                {
                    // The counter is shared across channels so the sample-and-hold
                    // stays phase-locked and the image does not wander.
                    holdCounter += 1.0f;
                    const auto refresh = holdCounter >= step;

                    if (refresh)
                        holdCounter -= step;

                    for (size_t channel = 0; channel < numChannels; ++channel)
                    {
                        auto* samples = block.getChannelPointer (channel);

                        if (refresh)
                        {
                            const auto quantised = std::round (samples[i] * levels) / levels;
                            held[channel] = quantised;
                        }

                        samples[i] = held[channel] * output;
                    }
                }
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Bits", "Rate", "Output" };
            }

        private:
            double             sampleRate  = 44100.0;
            float              levels      = 65535.0f;
            float              step        = 1.0f;
            float              holdCounter = 0.0f;
            float              output      = 1.0f;
            std::vector<float> held;
        };

        /** A: rate, B: depth, C: feedback.

            A flanger is a chorus with a much shorter delay and far more
            feedback, which is what turns the pitch wobble into a comb sweep.
        */
        class FlangerEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                chorus.prepare (spec);
                reset();
            }

            void reset() override { chorus.reset(); }

            void setParameters (float a, float b, float c) override
            {
                chorus.setRate (juce::jmap (a, 0.02f, 6.0f));
                chorus.setDepth (juce::jmap (b, 0.1f, 1.0f));
                chorus.setFeedback (juce::jmap (c, -0.95f, 0.95f));
                chorus.setCentreDelay (2.0f);   // chorus sits around 12 ms
                chorus.setMix (1.0f);
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                chorus.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Rate", "Depth", "Feedback" };
            }

        private:
            juce::dsp::Chorus<float> chorus;
        };

        /** A: rate, B: depth, C: feedback. */
        class ChorusEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                chorus.prepare (spec);
                reset();
            }

            void reset() override { chorus.reset(); }

            void setParameters (float a, float b, float c) override
            {
                chorus.setRate (juce::jmap (a, 0.05f, 8.0f));
                chorus.setDepth (b);
                chorus.setFeedback (juce::jmap (c, -0.9f, 0.9f));
                chorus.setCentreDelay (12.0f);
                chorus.setMix (1.0f);   // the rack handles wet/dry itself
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                chorus.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Rate", "Depth", "Feedback" };
            }

        private:
            juce::dsp::Chorus<float> chorus;
        };

        /** A: rate, B: depth, C: feedback. */
        class PhaserEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                phaser.prepare (spec);
                reset();
            }

            void reset() override { phaser.reset(); }

            void setParameters (float a, float b, float c) override
            {
                phaser.setRate (juce::jmap (a, 0.05f, 8.0f));
                phaser.setDepth (b);
                phaser.setFeedback (juce::jmap (c, -0.9f, 0.9f));
                phaser.setCentreFrequency (600.0f);
                phaser.setMix (1.0f);
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                phaser.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Rate", "Depth", "Feedback" };
            }

        private:
            juce::dsp::Phaser<float> phaser;
        };

        /** A: time, B: feedback, C: stereo offset. */
        class DelayEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                sampleRate = spec.sampleRate;

                delay.setMaximumDelayInSamples (static_cast<int> (spec.sampleRate * maxDelaySeconds) + 1);
                delay.prepare (spec);
                reset();
            }

            void reset() override { delay.reset(); }

            void setParameters (float a, float b, float c) override
            {
                delaySeconds = juce::jmap (a, 0.02f, maxDelaySeconds);
                feedback     = b * 0.95f;
                spread       = c;
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                const auto numChannels = block.getNumChannels();
                const auto numSamples  = block.getNumSamples();

                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    // Offsetting the right channel's delay time is what turns a
                    // mono echo into a stereo one.
                    const auto channelScale = (channel == 1) ? (1.0f - spread * 0.35f) : 1.0f;
                    const auto delaySamples = juce::jlimit (1.0f,
                                                            static_cast<float> (sampleRate * maxDelaySeconds),
                                                            delaySeconds * channelScale * static_cast<float> (sampleRate));

                    delay.setDelay (delaySamples);

                    auto* samples = block.getChannelPointer (channel);

                    for (size_t i = 0; i < numSamples; ++i)
                    {
                        const auto delayed = delay.popSample (static_cast<int> (channel));
                        delay.pushSample (static_cast<int> (channel), samples[i] + delayed * feedback);
                        samples[i] = delayed;
                    }
                }
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Time", "Feedback", "Spread" };
            }

        private:
            juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 96000 };
            double sampleRate   = 44100.0;
            float  delaySeconds = 0.25f;
            float  feedback     = 0.35f;
            float  spread       = 0.5f;
        };

        /** A: size, B: damping, C: width. */
        class ReverbEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                reverb.prepare (spec);
                reset();
            }

            void reset() override { reverb.reset(); }

            void setParameters (float a, float b, float c) override
            {
                juce::Reverb::Parameters p;
                p.roomSize   = a;
                p.damping    = b;
                p.width      = c;
                p.wetLevel   = 1.0f;   // the rack handles wet/dry itself
                p.dryLevel   = 0.0f;
                p.freezeMode = 0.0f;

                reverb.setParameters (p);
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                reverb.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Size", "Damping", "Width" };
            }

        private:
            juce::dsp::Reverb reverb;
        };

        /** A: low shelf gain, B: high shelf gain, C: tilt frequency. */
        class EqEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                sampleRate = spec.sampleRate;
                lowShelf.prepare (spec);
                highShelf.prepare (spec);
                reset();
            }

            void reset() override
            {
                lowShelf.reset();
                highShelf.reset();
            }

            void setParameters (float a, float b, float c) override
            {
                const auto nyquistLimit = static_cast<float> (sampleRate) * 0.45f;
                const auto centre       = juce::jlimit (100.0f, nyquistLimit, juce::jmap (c, 200.0f, 4000.0f));

                // 0.5 is flat; the ends give +/- 15 dB.
                const auto lowGain  = juce::Decibels::decibelsToGain ((a - 0.5f) * 30.0f);
                const auto highGain = juce::Decibels::decibelsToGain ((b - 0.5f) * 30.0f);

                *lowShelf.state  = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf  (sampleRate, centre, 0.7f, lowGain);
                *highShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (sampleRate, centre, 0.7f, highGain);
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                lowShelf.process (context);
                highShelf.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Low", "High", "Freq" };
            }

        private:
            using Duplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                              juce::dsp::IIR::Coefficients<float>>;

            double     sampleRate = 44100.0;
            Duplicator lowShelf;
            Duplicator highShelf;
        };

        /** A: threshold, B: ratio, C: release. */
        class CompressorEffect final : public Effect
        {
        public:
            void prepare (const juce::dsp::ProcessSpec& spec) override
            {
                compressor.prepare (spec);
                reset();
            }

            void reset() override { compressor.reset(); }

            void setParameters (float a, float b, float c) override
            {
                compressor.setThreshold (juce::jmap (a, 0.0f, -48.0f));
                compressor.setRatio (juce::jmap (b, 1.0f, 20.0f));
                compressor.setAttack (8.0f);
                compressor.setRelease (juce::jmap (c, 20.0f, 800.0f));
            }

            void process (juce::dsp::AudioBlock<float>& block) override
            {
                juce::dsp::ProcessContextReplacing<float> context (block);
                compressor.process (context);
            }

            std::array<const char*, 3> getControlNames() const override
            {
                return { "Threshold", "Ratio", "Release" };
            }

        private:
            juce::dsp::Compressor<float> compressor;
        };

        std::unique_ptr<Effect> createEffect (FXChain::Type type)
        {
            switch (type)
            {
                case FXChain::Type::Bypass:     return std::make_unique<BypassEffect>();
                case FXChain::Type::Distortion: return std::make_unique<DistortionEffect>();
                case FXChain::Type::BitCrusher: return std::make_unique<BitCrusherEffect>();
                case FXChain::Type::Chorus:     return std::make_unique<ChorusEffect>();
                case FXChain::Type::Flanger:    return std::make_unique<FlangerEffect>();
                case FXChain::Type::Phaser:     return std::make_unique<PhaserEffect>();
                case FXChain::Type::Delay:      return std::make_unique<DelayEffect>();
                case FXChain::Type::Reverb:     return std::make_unique<ReverbEffect>();
                case FXChain::Type::Eq:         return std::make_unique<EqEffect>();
                case FXChain::Type::Compressor: return std::make_unique<CompressorEffect>();
            }

            return std::make_unique<BypassEffect>();
        }
    }

    FXChain::FXChain()
    {
        // Built once here rather than when a slot's type changes, so that
        // switching effects never allocates on the audio thread.
        for (auto& slot : slots)
            for (int type = 0; type < numTypes; ++type)
                slot.effects[static_cast<size_t> (type)] = createEffect (static_cast<Type> (type));
    }

    FXChain::~FXChain() = default;

    void FXChain::prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& slot : slots)
            for (auto& effect : slot.effects)
                effect->prepare (spec);

        dryBuffer.setSize (static_cast<int> (spec.numChannels),
                           static_cast<int> (spec.maximumBlockSize),
                           false, false, true);
    }

    void FXChain::reset()
    {
        for (auto& slot : slots)
            for (auto& effect : slot.effects)
                effect->reset();

        dryBuffer.clear();
    }

    std::array<const char*, 3> FXChain::getControlNames (Type type) const
    {
        const auto index = juce::jlimit (0, numTypes - 1, static_cast<int> (type));
        return slots[0].effects[static_cast<size_t> (index)]->getControlNames();
    }

    void FXChain::process (juce::AudioBuffer<float>& buffer, const ParameterStore& parameters)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples  = buffer.getNumSamples();

        for (int i = 0; i < ids::numFxSlots; ++i)
        {
            const auto& slotParameters = parameters.fx[static_cast<size_t> (i)];

            if (! slotParameters.enable->get())
                continue;

            const auto type = static_cast<Type> (slotParameters.type->getIndex());

            if (type == Type::Bypass)
                continue;

            auto& effect = slots[static_cast<size_t> (i)].effects[static_cast<size_t> (type)];

            effect->setParameters (slotParameters.a->get(),
                                   slotParameters.b->get(),
                                   slotParameters.c->get());

            const auto mix = slotParameters.mix->get();

            // Effects run fully wet and the mix is applied here, which keeps
            // every slot's mix control behaving identically.
            for (int channel = 0; channel < numChannels; ++channel)
                dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

            juce::dsp::AudioBlock<float> block (buffer);
            auto subBlock = block.getSubBlock (0, static_cast<size_t> (numSamples));
            effect->process (subBlock);

            if (mix < 1.0f)
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    buffer.applyGain (channel, 0, numSamples, mix);
                    buffer.addFrom (channel, 0, dryBuffer, channel, 0, numSamples, 1.0f - mix);
                }
        }
    }
}
