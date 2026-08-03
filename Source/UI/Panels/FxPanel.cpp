#include "UI/Panels/FxPanel.h"

#include "Params/ParameterIDs.h"
#include "UI/Panels/SynthPanels.h"

namespace nog::ui
{
    namespace
    {
        constexpr int slotTitleHeight = 20;
        constexpr int slotHeaderHeight = 30;
        constexpr int knobRowHeight   = 62;
    }

    FxPanel::Slot::Slot (juce::AudioProcessorValueTreeState& stateToUse,
                         const fx::FXChain& chainToUse, int slotIndex)
        : enabled (stateToUse, ids::fx (slotIndex, ids::fxEnable), "ON"),
          type    (stateToUse, ids::fx (slotIndex, ids::fxType), {}),
          a       (stateToUse, ids::fx (slotIndex, ids::fxParamA), "A"),
          b       (stateToUse, ids::fx (slotIndex, ids::fxParamB), "B"),
          c       (stateToUse, ids::fx (slotIndex, ids::fxParamC), "C"),
          mix     (stateToUse, ids::fx (slotIndex, ids::fxMix), "Mix"),
          state   (stateToUse),
          chain   (chainToUse),
          index   (slotIndex)
    {
        title.setText ("SLOT " + juce::String (slotIndex + 1), juce::dontSendNotification);
        title.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        title.setColour (juce::Label::textColourId, colours::text);

        addAllChildren (*this, { &title, &enabled, &type, &a, &b, &c, &mix });

        // Relabel now so the knobs are correct before the first timer tick.
        timerCallback();
        startTimerHz (8);
    }

    void FxPanel::Slot::timerCallback()
    {
        auto* parameter = state.getRawParameterValue (ids::fx (index, ids::fxType));

        if (parameter == nullptr)
            return;

        const auto current = static_cast<int> (parameter->load());

        if (current == lastType)
            return;

        lastType = current;

        const auto names = chain.getControlNames (static_cast<fx::FXChain::Type> (current));

        a.setLabelText (names[0]);
        b.setLabelText (names[1]);
        c.setLabelText (names[2]);
    }

    void FxPanel::Slot::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (colours::panel);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
    }

    void FxPanel::Slot::resized()
    {
        auto bounds = getLocalBounds().reduced (6, 4);

        title.setBounds (bounds.removeFromTop (slotTitleHeight));

        auto headerRow = bounds.removeFromTop (slotHeaderHeight);
        enabled.setBounds (headerRow.removeFromLeft (46).reduced (2, 4));
        type.setBounds (headerRow.reduced (2, 4));

        // Centred vertically: a slot is taller than its four knobs need, and
        // leaving the gap below would make the strip look unfinished.
        layoutRow (bounds.withSizeKeepingCentre (bounds.getWidth(),
                                                 juce::jmin (bounds.getHeight(), knobRowHeight)),
                   { &a, &b, &c, &mix });
    }

    // -----------------------------------------------------------------------
    FxPanel::FxPanel (juce::AudioProcessorValueTreeState& state, const fx::FXChain& chain)
    {
        for (int i = 0; i < ids::numFxSlots; ++i)
        {
            auto slot = std::make_unique<Slot> (state, chain, i);
            addAndMakeVisible (*slot);
            slots[static_cast<size_t> (i)] = std::move (slot);
        }
    }

    void FxPanel::resized()
    {
        auto bounds = getLocalBounds().reduced (4);

        // Two rows of three. Slots run left to right, top to bottom, which is
        // also the order the audio passes through them.
        constexpr int columns = 3;
        const auto rows = (ids::numFxSlots + columns - 1) / columns;
        const auto rowHeight = bounds.getHeight() / juce::jmax (1, rows);

        for (int row = 0; row < rows; ++row)
        {
            auto rowArea = bounds.removeFromTop (rowHeight);
            const auto columnWidth = rowArea.getWidth() / columns;

            for (int column = 0; column < columns; ++column)
            {
                const auto index = row * columns + column;

                if (index >= ids::numFxSlots)
                    break;

                auto cell = column == columns - 1 ? rowArea : rowArea.removeFromLeft (columnWidth);
                slots[static_cast<size_t> (index)]->setBounds (cell.reduced (4));
            }
        }
    }
}
