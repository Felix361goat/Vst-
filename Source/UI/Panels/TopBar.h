#pragma once

#include "UI/Widgets.h"

namespace nog
{
    class NogSuiteProcessor;
}

namespace nog::ui
{
    /**
        The header strip: preset browser on the left, output metering and master
        level on the right.

        Owns the preset browsing UI because that is the only place it appears,
        and polls the processor for the voice count and output level.
    */
    class TopBar final : public juce::Component,
                         private juce::Timer
    {
    public:
        explicit TopBar (NogSuiteProcessor& processorToUse);

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;

        /** Rebuilds the preset list from disk and selects the current patch. */
        void refreshPresetList();

        void promptToSavePreset();

        /** Menu for choosing the background image and how far it is dimmed. */
        void showBackgroundMenu();
        void promptForBackgroundImage();

        NogSuiteProcessor& processor;

        juce::Label      logo;
        juce::TextButton previousButton { "<" };
        juce::TextButton nextButton     { ">" };
        juce::TextButton saveButton     { "SAVE" };
        juce::TextButton initButton     { "INIT" };
        juce::TextButton backgroundButton { "BG" };
        juce::ComboBox   presetList;

        // Held as a member because the chooser has to outlive the call that
        // launches it.
        std::unique_ptr<juce::FileChooser> fileChooser;

        juce::Label voiceCount;
        LevelMeter  meter;
        Knob        masterGain;

        int lastVoiceCount = -1;
    };
}
