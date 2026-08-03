#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace nog
{
    namespace
    {
        // These add up to the editor's minimum height, so every panel's controls
        // fit without clipping even when the window is at its smallest.
        constexpr int topBarHeight      = 56;
        constexpr int bottomBarHeight   = 244;
        constexpr int macroSectionWidth = 300;
        constexpr int oscRowHeight      = 252;

        using namespace nog::ui;

        /** The OSC tab: two oscillators over the sub, noise and filter row. */
        class SynthPage final : public juce::Component
        {
        public:
            SynthPage (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix)
                : osc1Panel (state, matrix, 0),
                  osc2Panel (state, matrix, 1),
                  subPanel (state, matrix),
                  noisePanel (state, matrix),
                  filterPanel (state, matrix)
            {
                osc1Section.setContent (osc1Panel);
                osc2Section.setContent (osc2Panel);
                subSection.setContent (subPanel);
                noiseSection.setContent (noisePanel);
                filterSection.setContent (filterPanel);

                for (auto* section : { &osc1Section, &osc2Section, &subSection,
                                       &noiseSection, &filterSection })
                    addAndMakeVisible (section);
            }

            void resized() override
            {
                auto bounds = getLocalBounds().reduced (6);

                // Fixed height for the oscillator row: its three knob rows need
                // the space, and the row below needs far less.
                auto oscRow = bounds.removeFromTop (juce::jmin (oscRowHeight, bounds.getHeight()));
                const auto half = oscRow.getWidth() / 2;

                osc1Section.setBounds (oscRow.removeFromLeft (half).reduced (3));
                osc2Section.setBounds (oscRow.reduced (3));

                bounds.removeFromTop (4);

                // Sub and noise are narrow; the filter takes the space left over
                // because it carries the most controls.
                const auto narrow = bounds.getWidth() / 4;

                subSection.setBounds (bounds.removeFromLeft (narrow).reduced (3));
                noiseSection.setBounds (bounds.removeFromLeft (narrow).reduced (3));
                filterSection.setBounds (bounds.reduced (3));
            }

        private:
            SectionPanel osc1Section   { "Oscillator 1" };
            SectionPanel osc2Section   { "Oscillator 2" };
            SectionPanel subSection    { "Sub" };
            SectionPanel noiseSection  { "Noise" };
            SectionPanel filterSection { "Filter" };

            OscillatorPanel osc1Panel, osc2Panel;
            SubPanel        subPanel;
            NoisePanel      noisePanel;
            FilterPanel     filterPanel;
        };

        /** The GLOBAL tab. */
        class GlobalPage final : public juce::Component
        {
        public:
            explicit GlobalPage (juce::AudioProcessorValueTreeState& state)
                : panel (state)
            {
                section.setContent (panel);
                addAndMakeVisible (section);
            }

            void resized() override
            {
                auto bounds = getLocalBounds().reduced (10);

                // Fixed size and left-aligned; stretching eight controls across
                // a wide window would only make them harder to hit.
                section.setBounds (bounds.removeFromTop (juce::jmin (200, bounds.getHeight()))
                                         .removeFromLeft (juce::jmin (620, bounds.getWidth())));
            }

        private:
            SectionPanel section { "Global" };
            GlobalPanel  panel;
        };
    }

    NogSuiteEditor::NogSuiteEditor (NogSuiteProcessor& processorToUse)
        : AudioProcessorEditor (&processorToUse),
          processor (processorToUse),
          topBar (processorToUse),
          fxPanel (processorToUse.getValueTreeState(), processorToUse.getEngine().getEffects()),
          matrixPanel (processorToUse.getValueTreeState()),
          modulators (processorToUse.getValueTreeState(), processorToUse.getEngine().getModMatrix()),
          macros (processorToUse.getValueTreeState())
    {
        setLookAndFeel (&lookAndFeel);

        synthPage = std::make_unique<SynthPage> (processor.getValueTreeState(),
                                                 processor.getEngine().getModMatrix());
        globalPage = std::make_unique<GlobalPage> (processor.getValueTreeState());

        mainTabs.setTabBarDepth (28);
        mainTabs.setOutline (0);
        mainTabs.addTab ("OSC",    ui::colours::background, synthPage.get(),  false);
        mainTabs.addTab ("FX",     ui::colours::background, &fxPanel,         false);
        mainTabs.addTab ("MATRIX", ui::colours::background, &matrixPanel,     false);
        mainTabs.addTab ("GLOBAL", ui::colours::background, globalPage.get(), false);

        addAndMakeVisible (topBar);
        addAndMakeVisible (mainTabs);
        addAndMakeVisible (modulators);

        macroSection.setContent (macros);
        addAndMakeVisible (macroSection);

        setResizable (true, true);
        setResizeLimits (minimumWidth, minimumHeight, 2400, 1600);

        // Reopen at whatever size the user last left the window.
        const auto saved = processor.getSavedEditorSize();
        const auto width  = saved.x >= minimumWidth  ? saved.x : defaultWidth;
        const auto height = saved.y >= minimumHeight ? saved.y : defaultHeight;

        setSize (width, height);
    }

    NogSuiteEditor::~NogSuiteEditor()
    {
        // Children must stop using the look and feel before it is destroyed.
        setLookAndFeel (nullptr);
    }

    void NogSuiteEditor::paint (juce::Graphics& g)
    {
        g.fillAll (ui::colours::background);
    }

    void NogSuiteEditor::resized()
    {
        processor.setSavedEditorSize ({ getWidth(), getHeight() });

        auto bounds = getLocalBounds();

        topBar.setBounds (bounds.removeFromTop (topBarHeight));

        auto bottom = bounds.removeFromBottom (bottomBarHeight);
        macroSection.setBounds (bottom.removeFromRight (macroSectionWidth).reduced (6, 6));
        modulators.setBounds (bottom.reduced (6, 6));

        mainTabs.setBounds (bounds);
    }
}
