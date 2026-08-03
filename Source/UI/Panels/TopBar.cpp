#include "UI/Panels/TopBar.h"

#include "Params/ParameterIDs.h"
#include "PluginProcessor.h"

namespace nog::ui
{
    namespace
    {
        constexpr int refreshHz = 20;
    }

    TopBar::TopBar (NogSuiteProcessor& processorToUse)
        : processor (processorToUse),
          masterGain (processorToUse.getValueTreeState(), ids::masterGain, "Master")
    {
        logo.setText (JucePlugin_Name, juce::dontSendNotification);
        logo.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
        logo.setColour (juce::Label::textColourId, colours::accent);
        addAndMakeVisible (logo);

        presetList.setTextWhenNothingSelected ("Init");
        presetList.onChange = [this]
        {
            const auto name = presetList.getText();

            // Ignore the change we cause ourselves when repopulating the list.
            if (name.isNotEmpty() && name != processor.getPresetManager().getCurrentPresetName())
                processor.getPresetManager().load (name);
        };
        addAndMakeVisible (presetList);

        previousButton.onClick = [this]
        {
            processor.getPresetManager().loadPrevious();
            refreshPresetList();
        };

        nextButton.onClick = [this]
        {
            processor.getPresetManager().loadNext();
            refreshPresetList();
        };

        saveButton.onClick = [this] { promptToSavePreset(); };

        initButton.onClick = [this]
        {
            processor.getPresetManager().loadDefault();
            refreshPresetList();
        };

        for (auto* button : { &previousButton, &nextButton, &saveButton, &initButton })
            addAndMakeVisible (button);

        voiceCount.setFont (juce::Font (juce::FontOptions (11.0f)));
        voiceCount.setColour (juce::Label::textColourId, colours::dimText);
        voiceCount.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (voiceCount);

        addAndMakeVisible (meter);
        addAndMakeVisible (masterGain);
        masterGain.showModulationFor (processor.getEngine().getModMatrix(), mod::Dest::MasterGain);

        refreshPresetList();
        startTimerHz (refreshHz);
    }

    void TopBar::refreshPresetList()
    {
        const auto names   = processor.getPresetManager().getPresetNames();
        const auto current = processor.getPresetManager().getCurrentPresetName();

        presetList.clear (juce::dontSendNotification);

        for (int i = 0; i < names.size(); ++i)
            presetList.addItem (names[i], i + 1);

        const auto index = names.indexOf (current);

        if (index >= 0)
            presetList.setSelectedId (index + 1, juce::dontSendNotification);
        else
            presetList.setText (current, juce::dontSendNotification);
    }

    void TopBar::promptToSavePreset()
    {
        auto* window = new juce::AlertWindow ("Save Preset",
                                              "Name this patch:",
                                              juce::MessageBoxIconType::NoIcon);

        window->addTextEditor ("name", processor.getPresetManager().getCurrentPresetName(), {});
        window->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        // Modal state with a callback keeps this safe on every host, including
        // the ones that will not tolerate a blocking modal loop.
        window->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, window] (int result)
            {
                const std::unique_ptr<juce::AlertWindow> owned (window);

                if (result != 1)
                    return;

                const auto name = owned->getTextEditorContents ("name").trim();

                if (name.isEmpty())
                    return;

                processor.getPresetManager().save (name);
                refreshPresetList();
            }), false);
    }

    void TopBar::timerCallback()
    {
        meter.setLevel (processor.getOutputLevel());

        const auto voices = processor.getActiveVoiceCount();

        if (voices == lastVoiceCount)
            return;

        lastVoiceCount = voices;
        voiceCount.setText (juce::String (voices) + " voices", juce::dontSendNotification);
    }

    void TopBar::paint (juce::Graphics& g)
    {
        g.setColour (colours::panel);
        g.fillRect (getLocalBounds());

        g.setColour (colours::border);
        g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
    }

    void TopBar::resized()
    {
        auto bounds = getLocalBounds().reduced (8, 6);

        logo.setBounds (bounds.removeFromLeft (130));

        // Master level lives at the far right, with the meter beneath it.
        masterGain.setBounds (bounds.removeFromRight (66));
        bounds.removeFromRight (8);

        auto meterArea = bounds.removeFromRight (150);
        voiceCount.setBounds (meterArea.removeFromTop (meterArea.getHeight() / 2).reduced (0, 2));
        meter.setBounds (meterArea.withSizeKeepingCentre (meterArea.getWidth(), 10));

        bounds.removeFromRight (12);

        // Preset controls, centred in what is left.
        auto presetArea = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), 420),
                                                        juce::jmin (bounds.getHeight(), 26));

        previousButton.setBounds (presetArea.removeFromLeft (30).reduced (1));
        nextButton.setBounds (presetArea.removeFromLeft (30).reduced (1));
        initButton.setBounds (presetArea.removeFromRight (52).reduced (1));
        saveButton.setBounds (presetArea.removeFromRight (52).reduced (1));
        presetList.setBounds (presetArea.reduced (4, 1));
    }
}
