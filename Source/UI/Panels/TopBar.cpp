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

        backgroundButton.onClick = [this] { showBackgroundMenu(); };
        backgroundButton.setTooltip ("Choose the background image");

        for (auto* button : { &previousButton, &nextButton, &saveButton, &initButton, &backgroundButton })
            addAndMakeVisible (button);

        voiceCount.setFont (juce::Font (juce::FontOptions (11.0f)));
        voiceCount.setColour (juce::Label::textColourId, colours::dimText);
        voiceCount.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (voiceCount);

        addAndMakeVisible (meter);
        masterGain.setAccentColour (colours::candyYellow);
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

    void TopBar::showBackgroundMenu()
    {
        const auto currentDim = processor.getBackgroundDim();
        const auto usingCustom = processor.getBackgroundImagePath().isNotEmpty();

        juce::PopupMenu dimming;

        // Named rather than a slider: three useful settings covers it, and a
        // menu needs no room in the header.
        const std::pair<const char*, float> levels[] {
            { "Show more of the image", 0.20f },
            { "Balanced",               0.42f },
            { "Favour readability",     0.65f }
        };

        for (int i = 0; i < 3; ++i)
            dimming.addItem (juce::PopupMenu::Item (levels[static_cast<size_t> (i)].first)
                                 .setTicked (std::abs (currentDim - levels[static_cast<size_t> (i)].second) < 0.01f)
                                 .setAction ([this, value = levels[static_cast<size_t> (i)].second]
                                             { processor.setBackgroundDim (value); }));

        juce::PopupMenu menu;
        menu.addSectionHeader ("Background");
        menu.addItem ("Choose image...", [this] { promptForBackgroundImage(); });
        menu.addItem (juce::PopupMenu::Item ("Use the built-in artwork")
                          .setEnabled (usingCustom)
                          .setAction ([this] { processor.setBackgroundImagePath ({}); }));
        menu.addSeparator();
        menu.addSubMenu ("Dimming", dimming);
        menu.addSeparator();
        menu.addItem (juce::PopupMenu::Item ("Open backgrounds folder")
                          .setAction ([]
                          {
                              const auto directory = NogSuiteProcessor::getBackgroundDirectory();

                              if (! directory.isDirectory())
                                  directory.createDirectory();

                              directory.revealToUser();
                          }));

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&backgroundButton));
    }

    void TopBar::promptForBackgroundImage()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Choose a background image",
            juce::File::getSpecialLocation (juce::File::userPicturesDirectory),
            "*.png;*.jpg;*.jpeg;*.gif;*.bmp");

        const auto browserFlags = juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (browserFlags, [this] (const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file == juce::File())
                return;

            // The image is copied into the plugin's own folder, so the
            // background survives the original being moved or deleted.
            if (! processor.chooseBackgroundImage (file))
                juce::NativeMessageBox::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "Could not use that image",
                    file.getFileName() + " could not be read as an image. "
                    "PNG, JPEG, GIF and BMP are supported.");
        });
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
        paintGlassPanel (g, getLocalBounds().toFloat().withTrimmedBottom (1.0f), 0.0f, true);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
    }

    void TopBar::resized()
    {
        auto bounds = getLocalBounds().reduced (8, 6);

        logo.setBounds (bounds.removeFromLeft (108));
        backgroundButton.setBounds (bounds.removeFromLeft (34).withSizeKeepingCentre (30, 22));
        bounds.removeFromLeft (6);

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
