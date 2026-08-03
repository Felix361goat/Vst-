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

        The whole interface is built once at a fixed logical size and then
        scaled by a transform to fill whatever size the window is dragged to.
        That is how the resize behaves the way a plugin is expected to: every
        control grows and shrinks together instead of the panels reflowing.
    */
    class NogSuiteEditor final : public juce::AudioProcessorEditor,
                                 public juce::DragAndDropContainer
    {
    public:
        /** The size the interface is designed at. Everything inside is laid out
            in these coordinates whatever the window is actually set to. */
        static constexpr int logicalWidth  = 1180;
        static constexpr int logicalHeight = 860;

        /** How far the window may be scaled from the design size. */
        static constexpr float minimumScale = 0.7f;
        static constexpr float maximumScale = 2.0f;

        explicit NogSuiteEditor (NogSuiteProcessor& processorToUse);
        ~NogSuiteEditor() override;

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        /** Everything visible lives inside this, so a single transform on it
            scales the entire interface.

            Also owns the background: the built-in artwork, or whatever image
            the user has chosen. It watches the plugin's state tree so a change
            made anywhere is picked up without the editor having to be told.
        */
        class Content final : public juce::Component,
                              private juce::ValueTree::Listener,
                              private juce::AsyncUpdater
        {
        public:
            explicit Content (NogSuiteProcessor& processorToUse);
            ~Content() override;

            void paint (juce::Graphics& g) override;

        private:
            void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& property) override;

            /** Reloading happens on the message thread, whichever thread the
                state change arrived on. */
            void handleAsyncUpdate() override;

            void reloadBackground();

            NogSuiteProcessor& processor;
            juce::Image        background;
            float              dim = 0.42f;
        };

        NogSuiteProcessor& processor;

        // Declared first so it outlives every component that uses it.
        ui::NogLookAndFeel lookAndFeel;

        Content content;

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

        // Shows the "drag onto a knob" hint on the modulation source handles.
        juce::TooltipWindow tooltips { this, 600 };

        // Keeps the window proportional while it is dragged.
        juce::ComponentBoundsConstrainer sizeConstrainer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NogSuiteEditor)
    };
}
