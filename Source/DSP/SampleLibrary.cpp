#include "DSP/SampleLibrary.h"

#include "DSP/SampleBank.h"

namespace nog::dsp
{
    SampleLibrary::SampleLibrary()
    {
        // WAV, AIFF, FLAC and Ogg out of the box; MP3 where the platform
        // provides a decoder.
        formats.registerBasicFormats();

        for (auto& slot : slots)
            slot.store (nullptr, std::memory_order_release);
    }

    bool SampleLibrary::loadIntoSlot (int slot, const juce::File& file)
    {
        if (! juce::isPositiveAndBelow (slot, numSlots))
            return false;

        auto sample = Sample::load (file, formats);

        if (sample == nullptr)
            return false;

        const auto index = static_cast<size_t> (slot);

        // Retained before it is published, so that the moment the audio thread
        // can see the pointer there is already an owner keeping it alive.
        retained.add (sample);
        current[index] = sample;

        slots[index].store (sample.get(), std::memory_order_release);
        return true;
    }

    bool SampleLibrary::loadBuiltIn (int slot, int builtInIndex)
    {
        if (! juce::isPositiveAndBelow (slot, numSlots))
            return false;

        auto sample = SampleBank::factory().get (builtInIndex);

        if (sample == nullptr)
            return false;

        const auto index = static_cast<size_t> (slot);

        retained.add (sample);
        current[index] = sample;

        slots[index].store (sample.get(), std::memory_order_release);
        return true;
    }

    void SampleLibrary::clearSlot (int slot)
    {
        if (! juce::isPositiveAndBelow (slot, numSlots))
            return;

        const auto index = static_cast<size_t> (slot);

        // Cleared first: after this store no voice will pick the sample up
        // again, and the ones mid-block still hold a pointer that stays valid
        // because `retained` never releases it.
        slots[index].store (nullptr, std::memory_order_release);
        current[index] = nullptr;
    }

    juce::String SampleLibrary::getSlotName (int slot) const
    {
        if (! juce::isPositiveAndBelow (slot, numSlots))
            return {};

        const auto& sample = current[static_cast<size_t> (slot)];
        return sample != nullptr ? sample->getName() : juce::String();
    }
}
