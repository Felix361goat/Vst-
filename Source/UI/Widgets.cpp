#include "UI/Widgets.h"

#include "Modulation/ModAssign.h"

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
    juce::String makeModSourceDragDescription (mod::Source source)
    {
        return "nog.modsource:" + juce::String (static_cast<int> (source));
    }

    mod::Source modSourceFromDragDescription (const juce::var& description)
    {
        const auto text = description.toString();

        if (! text.startsWith ("nog.modsource:"))
            return mod::Source::None;

        const auto index = text.fromFirstOccurrenceOf (":", false, false).getIntValue();

        if (index <= 0 || index >= mod::numSources)
            return mod::Source::None;

        return static_cast<mod::Source> (index);
    }

    // -----------------------------------------------------------------------
    ModSourceChip::ModSourceChip (mod::Source sourceToRepresent, juce::String labelText)
        : source (sourceToRepresent), text (std::move (labelText))
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Drag onto any knob to modulate it");
    }

    void ModSourceChip::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (highlighted ? colours::modulation.withAlpha (0.35f)
                                 : colours::modulation.withAlpha (0.18f));
        g.fillRoundedRectangle (bounds, 3.0f);

        g.setColour (colours::modulation);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        g.setFont (labelFont (10.5f, true));
        g.drawText (text, bounds, juce::Justification::centred, false);
    }

    void ModSourceChip::mouseEnter (const juce::MouseEvent&)
    {
        highlighted = true;
        repaint();
    }

    void ModSourceChip::mouseExit (const juce::MouseEvent&)
    {
        highlighted = false;
        repaint();
    }

    void ModSourceChip::mouseDrag (const juce::MouseEvent&)
    {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            if (! container->isDragAndDropActive())
                container->startDragging (makeModSourceDragDescription (source), this);
    }

    // -----------------------------------------------------------------------
    Knob::Knob (juce::AudioProcessorValueTreeState& stateToUse,
                const juce::String& parameterID,
                const juce::String& labelText)
        : state (stateToUse)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, valueBoxHeight);
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);

        // A wider drag distance makes fine adjustment possible without needing
        // a modifier key, which matters for cutoff and tuning controls.
        slider.setMouseDragSensitivity (200);
        slider.setVelocityBasedMode (false);

        addAndMakeVisible (slider);

        nameLabel.setText (labelText, juce::dontSendNotification);
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (labelFont (11.0f));
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        nameLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (nameLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, parameterID, slider);

        // The slider fills the knob, so right-clicks land on it rather than on
        // this component; listening to it is what lets the modulation menu open.
        slider.addMouseListener (this, false);
    }

    void Knob::mouseDown (const juce::MouseEvent& event)
    {
        if (event.mods.isPopupMenu())
            showModulationMenu();
    }

    bool Knob::isInterestedInDragSource (const SourceDetails& details)
    {
        return dest != mod::Dest::None
            && modSourceFromDragDescription (details.description) != mod::Source::None;
    }

    void Knob::itemDragEnter (const SourceDetails&)
    {
        dragOver = true;
        repaint();
    }

    void Knob::itemDragExit (const SourceDetails&)
    {
        dragOver = false;
        repaint();
    }

    void Knob::itemDropped (const SourceDetails& details)
    {
        dragOver = false;
        repaint();

        const auto source = modSourceFromDragDescription (details.description);

        if (source == mod::Source::None || dest == mod::Dest::None)
            return;

        if (mod::assign (state, source, dest) < 0)
        {
            // Every slot is taken. Saying so is better than silently ignoring
            // the drop and leaving the user wondering.
            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon,
                "Modulation matrix full",
                "All " + juce::String (ids::numMatrixSlots) + " slots are in use. "
                "Clear one on the MATRIX tab to make room.");
        }
    }

    void Knob::paintOverChildren (juce::Graphics& g)
    {
        if (! dragOver)
            return;

        g.setColour (colours::modulation);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }

    void Knob::showModulationMenu()
    {
        if (dest == mod::Dest::None)
            return;

        const auto slots = mod::findSlotsForDestination (state, dest);

        juce::PopupMenu menu;
        menu.addSectionHeader (juce::String (mod::destNames()[static_cast<size_t> (dest)]));

        if (slots.empty())
        {
            menu.addItem (juce::PopupMenu::Item ("No modulation assigned").setEnabled (false));
        }
        else
        {
            for (size_t i = 0; i < slots.size(); ++i)
            {
                const auto contents = mod::readSlot (state, slots[i]);
                const auto name = juce::String (mod::sourceNames()[static_cast<size_t> (contents.source)])
                                + "   " + juce::String (contents.amount * 100.0f, 0) + " %";

                menu.addItem (static_cast<int> (i) + 1, "Remove  " + name);
            }

            menu.addSeparator();
            menu.addItem (1000, "Remove all");
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this, slots] (int result)
                            {
                                if (result == 0)
                                    return;

                                if (result == 1000)
                                {
                                    mod::clearDestination (state, dest);
                                    return;
                                }

                                const auto index = static_cast<size_t> (result - 1);

                                if (index < slots.size())
                                    mod::clearSlot (state, slots[index]);
                            });
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

    void Knob::setAccentColour (juce::Colour newColour)
    {
        // Passed through a property rather than a member so the look and feel
        // can read it without knowing anything about this class.
        slider.getProperties().set (knobColourProperty,
                                    static_cast<juce::int64> (newColour.getARGB()));
        slider.repaint();
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
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
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

        paintGlassPanel (g, bounds, 6.0f, true);

        // A slightly denser band behind the title keeps it legible wherever the
        // artwork happens to be bright.
        {
            juce::Graphics::ScopedSaveState saved (g);

            juce::Path clip;
            clip.addRoundedRectangle (bounds, 6.0f);
            g.reduceClipRegion (clip);

            g.setColour (colours::panelHeader.withAlpha (0.55f));
            g.fillRect (bounds.withHeight (static_cast<float> (headerHeight)));
        }

        g.setColour (juce::Colours::white);
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
