#include "Modulation/ModAssign.h"

#include "Params/ParameterIDs.h"

namespace nog::mod
{
    namespace
    {
        template <typename ParameterType>
        ParameterType* find (juce::AudioProcessorValueTreeState& state, const juce::String& id)
        {
            return dynamic_cast<ParameterType*> (state.getParameter (id));
        }

        juce::AudioParameterChoice* sourceParam (juce::AudioProcessorValueTreeState& state, int slot)
        {
            return find<juce::AudioParameterChoice> (state, ids::matrix (slot, ids::modSource));
        }

        juce::AudioParameterChoice* destParam (juce::AudioProcessorValueTreeState& state, int slot)
        {
            return find<juce::AudioParameterChoice> (state, ids::matrix (slot, ids::modDest));
        }

        juce::AudioParameterFloat* amountParam (juce::AudioProcessorValueTreeState& state, int slot)
        {
            return find<juce::AudioParameterFloat> (state, ids::matrix (slot, ids::modAmount));
        }

        juce::AudioParameterBool* enableParam (juce::AudioProcessorValueTreeState& state, int slot)
        {
            return find<juce::AudioParameterBool> (state, ids::matrix (slot, ids::modEnable));
        }

        /** Writes a choice parameter by index, going through the host so the
            edit is seen as a user gesture. */
        void setChoice (juce::AudioParameterChoice* parameter, int index)
        {
            if (parameter == nullptr)
                return;

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (index)));
            parameter->endChangeGesture();
        }

        void setFloat (juce::AudioParameterFloat* parameter, float value)
        {
            if (parameter == nullptr)
                return;

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            parameter->endChangeGesture();
        }

        void setBool (juce::AudioParameterBool* parameter, bool value)
        {
            if (parameter == nullptr)
                return;

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (value ? 1.0f : 0.0f);
            parameter->endChangeGesture();
        }
    }

    SlotContents readSlot (juce::AudioProcessorValueTreeState& state, int slot)
    {
        SlotContents contents;

        if (! juce::isPositiveAndBelow (slot, ids::numMatrixSlots))
            return contents;

        if (auto* parameter = sourceParam (state, slot))
            contents.source = static_cast<Source> (parameter->getIndex());

        if (auto* parameter = destParam (state, slot))
            contents.dest = static_cast<Dest> (parameter->getIndex());

        if (auto* parameter = amountParam (state, slot))
            contents.amount = parameter->get();

        if (auto* parameter = enableParam (state, slot))
            contents.enabled = parameter->get();

        return contents;
    }

    int findFreeSlot (juce::AudioProcessorValueTreeState& state)
    {
        for (int slot = 0; slot < ids::numMatrixSlots; ++slot)
        {
            const auto contents = readSlot (state, slot);

            // A slot counts as free when it is not actually routing anything,
            // whichever half the user left empty.
            if (contents.source == Source::None || contents.dest == Dest::None)
                return slot;
        }

        return -1;
    }

    int findSlot (juce::AudioProcessorValueTreeState& state, Source source, Dest dest)
    {
        for (int slot = 0; slot < ids::numMatrixSlots; ++slot)
        {
            const auto contents = readSlot (state, slot);

            if (contents.source == source && contents.dest == dest)
                return slot;
        }

        return -1;
    }

    std::vector<int> findSlotsForDestination (juce::AudioProcessorValueTreeState& state, Dest dest)
    {
        std::vector<int> slots;

        if (dest == Dest::None)
            return slots;

        for (int slot = 0; slot < ids::numMatrixSlots; ++slot)
        {
            const auto contents = readSlot (state, slot);

            if (contents.dest == dest && contents.source != Source::None)
                slots.push_back (slot);
        }

        return slots;
    }

    int assign (juce::AudioProcessorValueTreeState& state, Source source, Dest dest, float amount)
    {
        if (source == Source::None || dest == Dest::None)
            return -1;

        // Dropping the same source on the same knob twice should adjust the
        // routing that is already there, not fill a second slot with a duplicate.
        auto slot = findSlot (state, source, dest);
        const auto reusing = slot >= 0;

        if (! reusing)
            slot = findFreeSlot (state);

        if (slot < 0)
            return -1;

        setChoice (sourceParam (state, slot), static_cast<int> (source));
        setChoice (destParam (state, slot), static_cast<int> (dest));
        setBool (enableParam (state, slot), true);

        // An existing routing keeps whatever depth the user had dialled in;
        // only a brand new one gets the default.
        if (! reusing)
            setFloat (amountParam (state, slot), amount);

        return slot;
    }

    void clearSlot (juce::AudioProcessorValueTreeState& state, int slot)
    {
        if (! juce::isPositiveAndBelow (slot, ids::numMatrixSlots))
            return;

        setChoice (sourceParam (state, slot), static_cast<int> (Source::None));
        setChoice (destParam (state, slot), static_cast<int> (Dest::None));
        setFloat (amountParam (state, slot), 0.0f);
    }

    void clearDestination (juce::AudioProcessorValueTreeState& state, Dest dest)
    {
        for (const auto slot : findSlotsForDestination (state, dest))
            clearSlot (state, slot);
    }
}
