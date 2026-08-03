#include "UI/Widgets.h"

namespace nog::ui
{
    namespace
    {
        constexpr int nameLabelHeight = 13;
        constexpr int valueBoxHeight  = 14;
        constexpr int modRefreshHz    = 15;

        juce::Font labelFont (float height, bool bold = false)
        {
            return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
        }
    }

    void addAllChildren (juce::Component& parent, std::initializer_list<juce::Component*> children)
    {
        for (auto* child : children)
            if (child != nullptr)
                parent.addAndMakeVisible (child);
    }

    // -----------------------------------------------------------------------
    Knob::Knob (juce::AudioProcessorValueTreeState& state,
                const juce::String& parameterID,
                const juce::String& labelText)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, valueBoxHeight);
        slider.setColour (juce::Slider::textBoxTextColourId, colours::dimText);

        // A wider drag distance makes fine adjustment possible without needing
        // a modifier key, which matters for cutoff and tuning controls.
        slider.setMouseDragSensitivity (200);
        slider.setVelocityBasedMode (false);

        addAndMakeVisible (slider);

        nameLabel.setText (labelText, juce::dontSendNotification);
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (labelFont (11.0f));
        nameLabel.setColour (juce::Label::textColourId, colours::dimText);
        nameLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (nameLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, parameterID, slider);
    }

    void Knob::showModulationFor (const ModMatrix& matrixToWatch, mod::Dest destination)
    {
        matrix = &matrixToWatch;
        dest   = destination;

        startTimerHz (modRefreshHz);
    }

    void Knob::setLabelText (const juce::String& newText)
    {
        nameLabel.setText (newText, juce::dontSendNotification);
    }

    void Knob::timerCallback()
    {
        if (matrix == nullptr)
            return;

        const auto depth = matrix->getModulationDepth (dest);

        // Only repaint when the ring actually changes; without this every
        // modulated knob would repaint fifteen times a second forever.
        if (std::abs (depth - lastDepth) < 1.0e-4f)
            return;

        lastDepth = depth;
        slider.getProperties().set ("modDepth", depth);
        slider.repaint();
    }

    void Knob::resized()
    {
        auto bounds = getLocalBounds();

        nameLabel.setBounds (bounds.removeFromTop (nameLabelHeight));

        // In a tall cell the rotary would otherwise float in the middle with a
        // large gap above its value box. Capping the height to roughly square
        // plus the value box keeps the control compact wherever it is placed.
        const auto preferredHeight = juce::jmin (bounds.getHeight(), bounds.getWidth() + valueBoxHeight);

        slider.setBounds (bounds.withHeight (preferredHeight));
    }

    // -----------------------------------------------------------------------
    ChoiceBox::ChoiceBox (juce::AudioProcessorValueTreeState& state,
                          const juce::String& parameterID,
                          const juce::String& labelText)
    {
        // The attachment syncs the selection but never fills the box, so the
        // items have to be copied from the parameter first - otherwise the
        // control comes up empty and nothing can be selected.
        if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (parameterID)))
            box.addItemList (parameter->choices, 1);
        else
            jassertfalse;   // not a choice parameter, or the ID is wrong

        addAndMakeVisible (box);

        nameLabel.setText (labelText, juce::dontSendNotification);
        nameLabel.setJustificationType (juce::Justification::centredLeft);
        nameLabel.setFont (labelFont (11.0f));
        nameLabel.setColour (juce::Label::textColourId, colours::dimText);
        addAndMakeVisible (nameLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            state, parameterID, box);
    }

    void ChoiceBox::resized()
    {
        auto bounds = getLocalBounds();

        if (nameLabel.getText().isNotEmpty())
            nameLabel.setBounds (bounds.removeFromTop (nameLabelHeight));

        box.setBounds (bounds.reduced (0, 1));
    }

    // -----------------------------------------------------------------------
    ToggleBox::ToggleBox (juce::AudioProcessorValueTreeState& state,
                          const juce::String& parameterID,
                          const juce::String& buttonText)
    {
        button.setButtonText (buttonText);
        addAndMakeVisible (button);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            state, parameterID, button);
    }

    void ToggleBox::resized()
    {
        button.setBounds (getLocalBounds());
    }

    // -----------------------------------------------------------------------
    SectionPanel::SectionPanel (juce::String titleText)
        : title (std::move (titleText))
    {
    }

    void SectionPanel::setContent (juce::Component& contentToShow)
    {
        content = &contentToShow;
        addAndMakeVisible (contentToShow);
        resized();
    }

    juce::Rectangle<int> SectionPanel::getContentBounds() const
    {
        return getLocalBounds().withTrimmedTop (headerHeight).reduced (6, 4);
    }

    void SectionPanel::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (colours::panel);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (colours::panelHeader);
        g.fillRoundedRectangle (bounds.withHeight (static_cast<float> (headerHeight) + 4.0f), 4.0f);
        g.fillRect (bounds.withY (static_cast<float> (headerHeight) - 4.0f).withHeight (4.0f));

        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        g.setColour (colours::text);
        g.setFont (labelFont (11.5f, true));
        g.drawText (title.toUpperCase(),
                    getLocalBounds().withHeight (headerHeight).reduced (8, 0),
                    juce::Justification::centredLeft, false);
    }

    void SectionPanel::resized()
    {
        if (content != nullptr)
            content->setBounds (getContentBounds());
    }

    // -----------------------------------------------------------------------
    void LevelMeter::setLevel (float newLevel)
    {
        // Decay slowly so a peak stays visible long enough to read, but follow
        // a rise immediately.
        const auto decayed = juce::jmax (newLevel, level * 0.82f);

        if (std::abs (decayed - level) < 1.0e-4f)
            return;

        level = decayed;
        repaint();
    }

    void LevelMeter::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (colours::knobTrack);
        g.fillRoundedRectangle (bounds, 2.0f);

        // Displayed on a decibel scale, since a linear peak meter spends almost
        // all of its travel in the top few dB.
        const auto decibels  = juce::Decibels::gainToDecibels (level, -48.0f);
        const auto proportion = juce::jlimit (0.0f, 1.0f, (decibels + 48.0f) / 48.0f);

        if (proportion > 0.0f)
        {
            g.setColour (level >= 1.0f ? colours::meterClip : colours::meter);
            g.fillRoundedRectangle (bounds.withWidth (bounds.getWidth() * proportion), 2.0f);
        }

        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 1.0f);
    }
}
