#pragma once

#include <array>
#include <atomic>

#include "DSP/Sample.h"

namespace nog::dsp
{
    /**
        The samples currently loaded into the synth, one slot per oscillator.

        The whole design exists to make one thing safe: replacing a sample while
        voices are reading the old one. Loading happens on the message thread and
        ends in a single atomic pointer store. The audio thread does one atomic
        load per block and then reads freely.

        Nothing is ever deleted while the plugin is alive. Every sample that has
        been loaded stays in `retained`, so a pointer the audio thread picked up
        cannot become dangling, and no destructor can ever run on the audio
        thread. The cost is that reloading repeatedly during one session
        accumulates memory - bounded by the 30 second per-sample cap, and
        released when the plugin is closed.
    */
    class SampleLibrary
    {
    public:
        static constexpr int numSlots = 2;   // one per oscillator

        SampleLibrary();

        /** Loads @p file into @p slot. Message thread only.
            Returns false if the file could not be decoded. */
        bool loadIntoSlot (int slot, const juce::File& file);

        /** Loads one of the generated character samples. Message thread only. */
        bool loadBuiltIn (int slot, int builtInIndex);

        /** Empties a slot. The oscillator falls back to its wavetable. */
        void clearSlot (int slot);

        /** The sample in @p slot, or nullptr. Safe on the audio thread. */
        const Sample* getSlot (int slot) const noexcept
        {
            if (! juce::isPositiveAndBelow (slot, numSlots))
                return nullptr;

            return slots[static_cast<size_t> (slot)].load (std::memory_order_acquire);
        }

        /** Name of the sample in @p slot, for the editor. Message thread. */
        juce::String getSlotName (int slot) const;

        juce::AudioFormatManager& getFormatManager() noexcept { return formats; }

    private:
        juce::AudioFormatManager formats;

        // Read by the audio thread; written only by the message thread.
        std::array<std::atomic<Sample*>, numSlots> slots {};

        // Keeps every loaded sample alive so the pointers above stay valid.
        juce::ReferenceCountedArray<Sample> retained;
        std::array<Sample::Ptr, numSlots> current;
    };
}
