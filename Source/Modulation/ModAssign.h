#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Modulation/ModDefs.h"

namespace nog::mod
{
    /**
        Editing the modulation matrix by parameter, rather than by hand.

        The matrix slots are ordinary host parameters, so assigning a routing is
        just setting four of them. These helpers put that in one place so the
        drag-and-drop code, the right-click menus and the matrix table all agree
        on what "a free slot" and "an existing routing" mean.

        Everything here runs on the message thread and writes through
        setValueNotifyingHost, so the host sees the change like any other edit
        and it lands in undo history and automation.
    */

    /** Default depth given to a routing created by dropping a source onto a
        knob. Enough to hear immediately, small enough not to be destructive. */
    inline constexpr float defaultDropAmount = 0.25f;

    /** Index of the first slot with nothing routed, or -1 if all 16 are used. */
    int findFreeSlot (juce::AudioProcessorValueTreeState& state);

    /** Slot already routing @p source to @p dest, or -1. */
    int findSlot (juce::AudioProcessorValueTreeState& state, Source source, Dest dest);

    /** Every slot routing to @p dest, whatever the source. */
    std::vector<int> findSlotsForDestination (juce::AudioProcessorValueTreeState& state, Dest dest);

    /** Routes @p source to @p dest.

        Reuses an existing slot for the same pair rather than adding a duplicate,
        so dropping the same source twice adjusts one routing instead of
        stacking two. Returns the slot used, or -1 when the matrix is full.
    */
    int assign (juce::AudioProcessorValueTreeState& state, Source source, Dest dest,
                float amount = defaultDropAmount);

    /** Empties a slot: source and destination back to None, amount to zero. */
    void clearSlot (juce::AudioProcessorValueTreeState& state, int slot);

    /** Empties every slot pointing at @p dest. */
    void clearDestination (juce::AudioProcessorValueTreeState& state, Dest dest);

    /** Reads a slot's source, destination and amount for display. */
    struct SlotContents
    {
        Source source = Source::None;
        Dest   dest   = Dest::None;
        float  amount = 0.0f;
        bool   enabled = false;
    };

    SlotContents readSlot (juce::AudioProcessorValueTreeState& state, int slot);
}
