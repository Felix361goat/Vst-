#include "UI/Widgets.h"

#include "DSP/WavetableBank.h"
#include "Modulation/ModAssign.h"
#include "Params/ParameterIDs.h"
#include "PluginProcessor.h"

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
    // -----------------------------------------------------------------------
    WaveDisplay::WaveDisplay (NogSuiteProcessor& processorToUse, int oscillatorIndex)
        : processor (processorToUse), index (oscillatorIndex)
    {
        setInterceptsMouseClicks (false, false);

        // Any rate works; the display is not animating anything, it is noticing
        // that a knob moved. Ten a second is below the threshold where a drag
        // feels laggy and well above what redrawing costs.
        oscillator.prepare (44100.0);
        startTimerHz (10);
    }

    void WaveDisplay::timerCallback()
    {
        if (refresh())
            repaint();
    }

    bool WaveDisplay::refresh()
    {
        auto& state = processor.getValueTreeState();

        const auto valueOf = [&state] (const juce::String& id)
        {
            auto* parameter = state.getParameter (id);
            return parameter != nullptr ? parameter->convertFrom0to1 (parameter->getValue()) : 0.0f;
        };

        const auto mode     = static_cast<int> (valueOf (ids::osc (index, ids::oscMode)));
        const auto wave     = static_cast<int> (valueOf (ids::osc (index, ids::oscWave)));
        const auto warpMode = static_cast<int> (valueOf (ids::osc (index, ids::oscWarpMode)));
        const auto morph    = valueOf (ids::osc (index, ids::oscWtPos));
        const auto warp     = valueOf (ids::osc (index, ids::oscWarpAmount));

        const auto* sample = processor.getEngine().getSampleLibrary().getSlot (index);

        if (mode == lastMode && wave == lastWave && warpMode == lastWarpMode
            && juce::approximatelyEqual (morph, lastMorph)
            && juce::approximatelyEqual (warp, lastWarp)
            && sample == lastSample)
            return false;

        lastMode = mode;
        lastWave = wave;
        lastWarpMode = warpMode;
        lastMorph = morph;
        lastWarp = warp;
        lastSample = sample;

        oscillator.setTable (&dsp::WavetableBank::factory().getTable (wave));
        oscillator.setSample (sample);

        dsp::Oscillator::Settings settings;
        settings.mode         = mode;
        settings.warpMode     = warpMode;
        settings.warpAmount   = warp;
        settings.morph        = morph;
        settings.level        = 1.0f;
        settings.unisonVoices = 1;
        oscillator.setSettings (settings);

        // The frame the morph control selects is resolved when a note starts,
        // not when the settings change, so without this the display would show
        // the first frame of the table whatever the knob said.
        oscillator.noteOn();

        showingSample = oscillator.isPlayingSample();
        sampleStart   = morph;

        if (showingSample)
            buildSamplePath();
        else
            buildWavetablePath();

        return true;
    }

    void WaveDisplay::buildWavetablePath()
    {
        path.clear();

        const auto bounds = getLocalBounds().toFloat().reduced (3.0f);

        if (bounds.isEmpty())
            return;

        constexpr int points = 256;

        for (int i = 0; i <= points; ++i)
        {
            const auto phase = static_cast<double> (i) / points;
            const auto value = juce::jlimit (-1.0f, 1.0f, oscillator.previewAt (phase));

            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (phase);
            const auto y = bounds.getCentreY() - value * bounds.getHeight() * 0.45f;

            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }
    }

    void WaveDisplay::buildSamplePath()
    {
        path.clear();

        const auto bounds = getLocalBounds().toFloat().reduced (3.0f);

        if (bounds.isEmpty() || lastSample == nullptr)
            return;

        // A peak envelope rather than the waveform: at this width one pixel
        // covers hundreds of samples, and drawing every other one would show a
        // shape that is not in the file.
        const auto columns = juce::jmax (1, static_cast<int> (bounds.getWidth()));
        constexpr int perColumn = 32;

        std::vector<float> tops (static_cast<size_t> (columns));
        std::vector<float> bottoms (static_cast<size_t> (columns));

        for (int c = 0; c < columns; ++c)
        {
            auto high = 0.0f;
            auto low  = 0.0f;

            for (int n = 0; n < perColumn; ++n)
            {
                const auto position = (c + static_cast<double> (n) / perColumn) / columns;
                const auto value = lastSample->read (0, position);

                high = juce::jmax (high, value);
                low  = juce::jmin (low, value);
            }

            tops[static_cast<size_t> (c)]    = juce::jlimit (-1.0f, 1.0f, high);
            bottoms[static_cast<size_t> (c)] = juce::jlimit (-1.0f, 1.0f, low);
        }

        const auto yFor = [&bounds] (float value)
        {
            return bounds.getCentreY() - value * bounds.getHeight() * 0.45f;
        };

        path.startNewSubPath (bounds.getX(), yFor (tops[0]));

        for (int c = 1; c < columns; ++c)
            path.lineTo (bounds.getX() + c, yFor (tops[static_cast<size_t> (c)]));

        for (int c = columns - 1; c >= 0; --c)
            path.lineTo (bounds.getX() + c, yFor (bottoms[static_cast<size_t> (c)]));

        path.closeSubPath();
    }

    void WaveDisplay::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine (static_cast<int> (bounds.getCentreY()), bounds.getX(), bounds.getRight());

        if (showingSample)
        {
            g.setColour (accent.withAlpha (0.55f));
            g.fillPath (path);

            // Where playback begins. On a one-shot that is the difference
            // between hearing the attack and starting halfway through the tail.
            const auto x = bounds.getX() + bounds.getWidth() * juce::jlimit (0.0f, 1.0f, sampleStart);

            g.setColour (juce::Colours::white.withAlpha (0.65f));
            g.drawVerticalLine (static_cast<int> (x), bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);
        }
        else
        {
            g.setColour (accent);
            g.strokePath (path, juce::PathStrokeType (1.6f));
        }

        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
    }
}
