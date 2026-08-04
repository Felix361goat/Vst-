#include "PluginEditor.h"

#include <BinaryData.h>

#include "PluginProcessor.h"

namespace nog
{
    namespace
    {
        // These add up to the editor's minimum height, so every panel's controls
        // fit without clipping even when the window is at its smallest.
        constexpr int topBarHeight      = 56;
        constexpr int bottomBarHeight   = 268;
        constexpr int macroSectionWidth = 300;
        constexpr int oscRowHeight      = 292;

        using namespace nog::ui;

        /** The OSC tab: two oscillators over the sub, noise and filter row. */
        class SynthPage final : public juce::Component
        {
        public:
            SynthPage (NogSuiteProcessor& processor, const ModMatrix& matrix)
                : osc1Panel (processor, matrix, 0),
                  osc2Panel (processor, matrix, 1),
                  subPanel (processor.getValueTreeState(), matrix),
                  noisePanel (processor.getValueTreeState(), matrix),
                  filterPanel (processor.getValueTreeState(), matrix)
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
                : panel (state), arpPanel (state)
            {
                section.setContent (panel);
                arpSection.setContent (arpPanel);
                addAllChildren (*this, { &section, &arpSection });
            }

            void resized() override
            {
                auto bounds = getLocalBounds().reduced (10);

                // Fixed size and left-aligned; stretching eight controls across
                // a wide window would only make them harder to hit.
                auto row = bounds.removeFromTop (juce::jmin (200, bounds.getHeight()));
                section.setBounds (row.removeFromLeft (juce::jmin (620, row.getWidth())));
                row.removeFromLeft (12);
                arpSection.setBounds (row.removeFromLeft (juce::jmin (480, row.getWidth())));
            }

        private:
            SectionPanel section { "Global" };
            SectionPanel arpSection { "Arpeggiator" };
            GlobalPanel  panel;
            ArpPanel     arpPanel;
        };
    }

    NogSuiteEditor::Content::Content (NogSuiteProcessor& processorToUse)
        : processor (processorToUse)
    {
        // Children paint themselves; this only draws behind them.
        setInterceptsMouseClicks (false, true);

        reloadBackground();
        processor.getValueTreeState().state.addListener (this);
    }

    NogSuiteEditor::Content::~Content()
    {
        processor.getValueTreeState().state.removeListener (this);
        cancelPendingUpdate();
    }

    void NogSuiteEditor::Content::valueTreePropertyChanged (juce::ValueTree&,
                                                            const juce::Identifier& property)
    {
        // Only the two that affect the background; the tree also carries the
        // editor size and the preset name.
        if (property.toString() == "backgroundImage" || property.toString() == "backgroundDim")
            triggerAsyncUpdate();
    }

    void NogSuiteEditor::Content::handleAsyncUpdate()
    {
        reloadBackground();
        repaint();
    }

    void NogSuiteEditor::Content::reloadBackground()
    {
        dim = processor.getBackgroundDim();

        const auto path = processor.getBackgroundImagePath();

        if (path.isNotEmpty())
        {
            const juce::File file (path);

            if (file.existsAsFile())
            {
                auto loaded = juce::ImageCache::getFromFile (file);

                if (loaded.isValid())
                {
                    // A photo straight off a phone can be 4000 pixels wide.
                    // Rescaling once here costs far less than resampling it on
                    // every repaint.
                    constexpr int maximumWidth = 2048;

                    if (loaded.getWidth() > maximumWidth)
                        loaded = loaded.rescaled (maximumWidth,
                                                  loaded.getHeight() * maximumWidth / loaded.getWidth(),
                                                  juce::Graphics::highResamplingQuality);

                    background = loaded;
                    return;
                }
            }
        }

        // No choice made, or the file has gone: fall back to the artwork that
        // ships inside the binary, which cannot go missing.
        background = juce::ImageCache::getFromMemory (BinaryData::background_jpg,
                                                      BinaryData::background_jpgSize);
    }

    void NogSuiteEditor::Content::paint (juce::Graphics& g)
    {
        g.fillAll (ui::colours::background);

        if (background.isValid())
        {
            // Fills the window, cropping rather than squashing, so the image
            // keeps its proportions at any window shape.
            g.drawImage (background, getLocalBounds().toFloat(),
                         juce::RectanglePlacement::fillDestination);

            // Knocked back so white text on the translucent panels stays
            // readable however bright the chosen image happens to be.
            g.setColour (ui::colours::background.withAlpha (dim));
            g.fillRect (getLocalBounds());
        }
    }

    // -----------------------------------------------------------------------
    NogSuiteEditor::NogSuiteEditor (NogSuiteProcessor& processorToUse)
        : AudioProcessorEditor (&processorToUse),
          processor (processorToUse),
          content (processorToUse),
          topBar (processorToUse),
          fxPanel (processorToUse.getValueTreeState(), processorToUse.getEngine().getEffects()),
          matrixPanel (processorToUse.getValueTreeState()),
          modulators (processorToUse, processorToUse.getEngine().getModMatrix()),
          macros (processorToUse.getValueTreeState())
    {
        setLookAndFeel (&lookAndFeel);

        addAndMakeVisible (content);

        synthPage = std::make_unique<SynthPage> (processor, processor.getEngine().getModMatrix());
        globalPage = std::make_unique<GlobalPage> (processor.getValueTreeState());

        mainTabs.setTabBarDepth (28);
        mainTabs.setOutline (0);
        // Transparent, or the tab's own background would paint over the
        // artwork and undo the whole point of the translucent panels.
        mainTabs.addTab ("OSC",    juce::Colours::transparentBlack, synthPage.get(),  false);
        mainTabs.addTab ("FX",     juce::Colours::transparentBlack, &fxPanel,         false);
        mainTabs.addTab ("MATRIX", juce::Colours::transparentBlack, &matrixPanel,     false);
        mainTabs.addTab ("GLOBAL", juce::Colours::transparentBlack, globalPage.get(), false);

        // Everything goes inside the content component, which is what gets
        // scaled; adding to the editor directly would leave a control at a
        // fixed size while the rest of the interface grew.
        content.addAndMakeVisible (topBar);
        content.addAndMakeVisible (mainTabs);
        content.addAndMakeVisible (modulators);

        macroSection.setContent (macros);
        content.addAndMakeVisible (macroSection);

        setResizable (true, true);

        // Locking the aspect ratio is what makes the corner drag scale the
        // interface rather than reshape it.
        sizeConstrainer.setFixedAspectRatio (static_cast<double> (logicalWidth)
                                             / static_cast<double> (logicalHeight));
        sizeConstrainer.setSizeLimits (juce::roundToInt (logicalWidth * minimumScale),
                                       juce::roundToInt (logicalHeight * minimumScale),
                                       juce::roundToInt (logicalWidth * maximumScale),
                                       juce::roundToInt (logicalHeight * maximumScale));
        setConstrainer (&sizeConstrainer);

        // Reopen at whatever size the user last left the window.
        const auto saved = processor.getSavedEditorSize();
        const auto smallest = juce::roundToInt (logicalWidth * minimumScale);

        setSize (saved.x >= smallest ? saved.x : logicalWidth,
                 saved.y >= juce::roundToInt (logicalHeight * minimumScale) ? saved.y : logicalHeight);
    }

    NogSuiteEditor::~NogSuiteEditor()
    {
        // Children must stop using the look and feel before it is destroyed.
        setLookAndFeel (nullptr);
    }

    void NogSuiteEditor::paint (juce::Graphics& g)
    {
        // Only ever visible in the sliver the aspect ratio cannot fill exactly.
        g.fillAll (ui::colours::background);
    }

    void NogSuiteEditor::resized()
    {
        processor.setSavedEditorSize ({ getWidth(), getHeight() });

        // The interface is always laid out at its design size; the transform
        // does the resizing. Scaling by width alone is safe because the
        // constrainer holds the aspect ratio.
        const auto scale = static_cast<float> (getWidth()) / static_cast<float> (logicalWidth);

        content.setTransform (juce::AffineTransform::scale (scale));
        content.setBounds (0, 0, logicalWidth, logicalHeight);

        auto bounds = content.getLocalBounds();

        topBar.setBounds (bounds.removeFromTop (topBarHeight));

        auto bottom = bounds.removeFromBottom (bottomBarHeight);
        macroSection.setBounds (bottom.removeFromRight (macroSectionWidth).reduced (6, 6));
        modulators.setBounds (bottom.reduced (6, 6));

        mainTabs.setBounds (bounds);
    }
}
