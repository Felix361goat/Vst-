#pragma once

#include <array>
#include <memory>
#include <juce_dsp/juce_dsp.h>

#include "Params/ParameterStore.h"

namespace nog::fx
{
    /**
        Base class for a slot effect.

        Every effect is driven by the same four controls - three generic
        parameters plus a wet/dry mix - so the rack can be re-ordered and
        re-typed without the parameter surface changing shape. What A, B and C
        mean is documented per effect in FXChain.cpp and shown in the editor.
    */
    class Effect
    {
    public:
        virtual ~Effect() = default;

        virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
        virtual void reset() = 0;

        /** All values are 0..1; each effect maps them into its own units. */
        virtual void setParameters (float a, float b, float c) = 0;

        virtual void process (juce::dsp::AudioBlock<float>& block) = 0;

        /** Display names for this effect's three controls. */
        virtual std::array<const char*, 3> getControlNames() const = 0;
    };

    /**
        A fixed rack of effect slots.

        Every effect type is constructed up front for every slot, so changing a
        slot's type on the audio thread is a pointer swap rather than an
        allocation. That costs some memory in exchange for never blocking the
        audio thread, which is the right trade for a plugin.
    */
    class FXChain
    {
    public:
        // Order must match params::choices::effectTypes(). New types go on the
        // end: the value is a choice index that ends up in saved patches.
        enum class Type
        {
            Bypass = 0, Distortion, BitCrusher, Chorus, Flanger,
            Phaser, Delay, Reverb, Eq, Compressor
        };

        static constexpr int numTypes = 10;

        FXChain();
        ~FXChain();

        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();

        void process (juce::AudioBuffer<float>& buffer, const ParameterStore& parameters);

        /** Control names for a type, for the editor to label its knobs. */
        std::array<const char*, 3> getControlNames (Type type) const;

    private:
        struct Slot
        {
            std::array<std::unique_ptr<Effect>, numTypes> effects;
        };

        std::array<Slot, ids::numFxSlots> slots;
        juce::AudioBuffer<float> dryBuffer;
    };
}
