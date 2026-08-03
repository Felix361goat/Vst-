#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Modulation/ModMatrix.h"
#include "UI/NogLookAndFeel.h"

namespace nog::ui
{
    /** Adds every child to @p parent and makes it visible.

        The explicit juce::Component* element type is what makes this usable with
        a braced list of mixed widget types, which a plain range-for over an
        initializer list cannot deduce.
    */
    void addAllChildren (juce::Component& parent, std::initializer_list<juce::Component*> children);

    /**
        A labelled rotary control bound to a parameter.

        Optionally shows how far the modulation matrix can push the parameter, by
        polling the matrix on a timer and handing the depth to the look and feel
        through a slider property.
    */
    class Knob final : public juce::Component,
                       private juce::Timer
    {
    public:
        Knob (juce::AudioProcessorValueTreeState& state,
              const juce::String& parameterID,
              const juce::String& labelText);

        /** Starts showing the modulation ring for @p destination. */
        void showModulationFor (const ModMatrix& matrixToWatch, mod::Dest destination);

        /** Renames the control. Used by the FX rack, where what a knob does
            depends on the effect type selected in its slot. */
        void setLabelText (const juce::String& newText);

        void resized() override;

        juce::Slider slider;

    private:
        void timerCallback() override;

        juce::Label nameLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

        const ModMatrix* matrix = nullptr;
        mod::Dest        dest   = mod::Dest::None;
        float            lastDepth = 0.0f;
    };

    /** A labelled combo box bound to a choice parameter. */
    class ChoiceBox final : public juce::Component
    {
    public:
        ChoiceBox (juce::AudioProcessorValueTreeState& state,
                   const juce::String& parameterID,
                   const juce::String& labelText);

        void resized() override;

        juce::ComboBox box;

    private:
        juce::Label nameLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    /** A toggle bound to a bool parameter. */
    class ToggleBox final : public juce::Component
    {
    public:
        ToggleBox (juce::AudioProcessorValueTreeState& state,
                   const juce::String& parameterID,
                   const juce::String& buttonText);

        void resized() override;

        juce::ToggleButton button;

    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    /** A titled container that draws the panel background. */
    class SectionPanel final : public juce::Component
    {
    public:
        explicit SectionPanel (juce::String titleText);

        /** Adds a control that the panel will lay out; the panel does not own
            it, so callers keep their controls as members. */
        void setContent (juce::Component& contentToShow);

        void paint (juce::Graphics& g) override;
        void resized() override;

        /** The area inside the header and border, for callers laying out
            content themselves. */
        juce::Rectangle<int> getContentBounds() const;

        static constexpr int headerHeight = 22;

    private:
        juce::String title;
        juce::Component* content = nullptr;
    };

    /** A horizontal peak meter fed by the processor. */
    class LevelMeter final : public juce::Component
    {
    public:
        void setLevel (float newLevel);
        void paint (juce::Graphics& g) override;

    private:
        float level = 0.0f;
    };
}
