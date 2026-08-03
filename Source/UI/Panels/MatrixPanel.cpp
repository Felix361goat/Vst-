#include "UI/Panels/MatrixPanel.h"

#include "Params/ParameterIDs.h"

namespace nog::ui
{
    namespace
    {
        // Sixteen rows plus the header have to fit the editor's minimum height.
        constexpr int rowHeight    = 24;
        constexpr int headerHeight = 20;
        constexpr int numberWidth  = 26;
        constexpr int toggleWidth  = 42;
        constexpr int bipolarWidth = 58;
    }

    MatrixPanel::Row::Row (juce::AudioProcessorValueTreeState& state, int index)
        : enabled     (state, ids::matrix (index, ids::modEnable), "ON"),
          source      (state, ids::matrix (index, ids::modSource), {}),
          destination (state, ids::matrix (index, ids::modDest), {}),
          via         (state, ids::matrix (index, ids::modVia), {}),
          bipolar     (state, ids::matrix (index, ids::modBipolar), "BIPOLAR")
    {
        number.setText (juce::String (index + 1), juce::dontSendNotification);
        number.setJustificationType (juce::Justification::centred);
        number.setFont (juce::Font (juce::FontOptions (11.0f)));
        number.setColour (juce::Label::textColourId, colours::dimText);

        amount.setSliderStyle (juce::Slider::LinearHorizontal);
        amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 20);
        amount.setColour (juce::Slider::textBoxTextColourId, colours::dimText);

        amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, ids::matrix (index, ids::modAmount), amount);
    }

    MatrixPanel::MatrixPanel (juce::AudioProcessorValueTreeState& state)
    {
        const auto headerNames = std::array { "SOURCE", "VIA", "DESTINATION", "AMOUNT" };
        const auto headerLabels = std::array { &sourceHeader, &viaHeader, &destinationHeader, &amountHeader };

        for (size_t i = 0; i < headerLabels.size(); ++i)
        {
            auto& label = *headerLabels[i];

            label.setText (headerNames[i], juce::dontSendNotification);
            label.setFont (juce::Font (juce::FontOptions (10.0f)));
            label.setColour (juce::Label::textColourId, colours::dimText);
            label.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (label);
        }

        for (int i = 0; i < ids::numMatrixSlots; ++i)
        {
            auto row = std::make_unique<Row> (state, i);
            addRowComponents (*row);
            rows[static_cast<size_t> (i)] = std::move (row);
        }
    }

    void MatrixPanel::addRowComponents (Row& row)
    {
        addAndMakeVisible (row.number);
        addAndMakeVisible (row.enabled);
        addAndMakeVisible (row.source);
        addAndMakeVisible (row.via);
        addAndMakeVisible (row.destination);
        addAndMakeVisible (row.amount);
        addAndMakeVisible (row.bipolar);
    }

    void MatrixPanel::paint (juce::Graphics& g)
    {
        paintGlassPanel (g, getLocalBounds().toFloat().reduced (2.0f), 6.0f, true);

        // Alternating row tints make a sixteen-row table scannable.
        auto bounds = getLocalBounds().reduced (4);
        bounds.removeFromTop (headerHeight);

        for (int i = 0; i < ids::numMatrixSlots; ++i)
        {
            const auto row = bounds.removeFromTop (rowHeight);

            if (i % 2 == 0)
            {
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 1.0f), 3.0f);
            }
        }
    }

    MatrixPanel::Columns MatrixPanel::splitIntoColumns (juce::Rectangle<int> area)
    {
        Columns columns;

        columns.number  = area.removeFromLeft (numberWidth);
        columns.enabled = area.removeFromLeft (toggleWidth);
        columns.bipolar = area.removeFromRight (bipolarWidth);

        // Source, via, destination and amount share what is left, so the table
        // stays proportional when the window is resized. Via is narrower than
        // the rest: most slots leave it empty.
        const auto cell = area.getWidth() / 7;

        columns.source      = area.removeFromLeft (cell * 2);
        columns.via         = area.removeFromLeft (cell);
        columns.destination = area.removeFromLeft (cell * 2);
        columns.amount      = area;

        return columns;
    }

    void MatrixPanel::resized()
    {
        auto bounds = getLocalBounds().reduced (4);

        const auto headerColumns = splitIntoColumns (bounds.removeFromTop (headerHeight));

        sourceHeader.setBounds (headerColumns.source.withTrimmedLeft (4));
        viaHeader.setBounds (headerColumns.via.withTrimmedLeft (4));
        destinationHeader.setBounds (headerColumns.destination.withTrimmedLeft (4));
        amountHeader.setBounds (headerColumns.amount.withTrimmedLeft (4));

        for (auto& row : rows)
        {
            if (row == nullptr)
                continue;

            const auto columns = splitIntoColumns (bounds.removeFromTop (rowHeight).reduced (2, 2));

            row->number.setBounds (columns.number);
            row->enabled.setBounds (columns.enabled.reduced (2, 1));
            row->source.setBounds (columns.source.reduced (2, 1));
            row->via.setBounds (columns.via.reduced (2, 1));
            row->destination.setBounds (columns.destination.reduced (2, 1));
            row->amount.setBounds (columns.amount.reduced (2, 1));
            row->bipolar.setBounds (columns.bipolar.reduced (2, 1));
        }
    }
}
