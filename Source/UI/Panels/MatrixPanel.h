#pragma once

#include <array>
#include <memory>

#include "UI/Widgets.h"

namespace nog::ui
{
    /**
        The modulation matrix, one row per slot.

        Sixteen rows of four controls is a lot of components, but they are all
        parameter-attached, so the panel itself holds no state and nothing has to
        be kept in sync by hand.
    */
    class MatrixPanel final : public juce::Component
    {
    public:
        explicit MatrixPanel (juce::AudioProcessorValueTreeState& state);

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        struct Row
        {
            Row (juce::AudioProcessorValueTreeState& state, int index);

            ToggleBox enabled;
            ChoiceBox source;
            ChoiceBox destination;
            ToggleBox bipolar;
            juce::Slider amount;
            juce::Label  number;

            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
        };

        void addRowComponents (Row& row);

        /** Splits @p area into the same columns a row uses, so the headers line
            up with the controls they describe at any window width. */
        struct Columns
        {
            juce::Rectangle<int> number, enabled, source, destination, amount, bipolar;
        };

        static Columns splitIntoColumns (juce::Rectangle<int> area);

        std::array<std::unique_ptr<Row>, ids::numMatrixSlots> rows;

        juce::Label sourceHeader, destinationHeader, amountHeader;
    };
}
