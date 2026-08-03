#pragma once

#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/NogLookAndFeel.h"
#include "UI/Panels/FxPanel.h"
#include "UI/Panels/MatrixPanel.h"
#include "UI/Panels/ModulatorPanels.h"
#include "UI/Panels/SynthPanels.h"
#include "UI/Panels/TopBar.h"

namespace nog
{
    class NogSuiteProcessor;

    /**
        The plugin window.

        Laid out as three bands: a header with the preset browser and master
        level, a tabbed working area, and a modulator strip along the bottom
        that stays visible whichever tab is open - envelopes and LFOs are needed
        while editing every other page.
    */
    class NogSuiteEditor final : public juce::AudioProcessorEditor
    {
    public:
        // The minimum is the size at which every panel still fits its controls;
        // the constrainer stops the window going below it.
        static constexpr int defaultWidth  = 1180;
        static constexpr int defaultHeight = 860;
        static constexpr int minimumWidth  = 960;
        static constexpr int minimumHeight = 760;

        explicit NogSuiteEditor (NogSuiteProcessor& processorToUse);
        ~NogSuiteEditor() override;

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        NogSuiteProcessor& processor;

        // Declared first so it outlives every component that uses it.
        ui::NogLookAndFeel lookAndFeel;

        ui::TopBar topBar;

        juce::TabbedComponent mainTabs { juce::TabbedButtonBar::TabsAtTop };

        // The oscillator page is defined in the .cpp, since nothing else needs
        // to know its layout.
        std::unique_ptr<juce::Component> synthPage;
        std::unique_ptr<juce::Component> globalPage;

        ui::FxPanel     fxPanel;
        ui::MatrixPanel matrixPanel;

        ui::ModulatorsPanel modulators;
        ui::SectionPanel    macroSection { "Macros" };
        ui::MacroStrip      macros;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NogSuiteEditor)
    };
}
