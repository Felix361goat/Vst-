#pragma once

#include <array>
#include <memory>

#include "DSP/Lfo.h"
#include "UI/Widgets.h"

namespace nog::ui
{
    /** Draws the current AHDSR shape, refreshed from the parameters. */
    class EnvelopeDisplay final : public juce::Component,
                                  private juce::Timer
    {
    public:
        EnvelopeDisplay (juce::AudioProcessorValueTreeState& state, int envelopeIndex);

        void paint (juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::AudioProcessorValueTreeState& state;
        int index;

        // Cached so the display only repaints when something actually moved.
        std::array<float, 8> lastValues {};
    };

    class EnvelopePanel final : public juce::Component
    {
    public:
        EnvelopePanel (juce::AudioProcessorValueTreeState& state, int index);

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        EnvelopeDisplay display;
        ModSourceChip   dragHandle;
        Knob attack, hold, decay, sustain, release;
        Knob attackCurve, decayCurve, releaseCurve;
    };

    /** Draws one cycle of the selected LFO shape. */
    class LfoDisplay final : public juce::Component,
                             private juce::Timer
    {
    public:
        LfoDisplay (juce::AudioProcessorValueTreeState& state, int lfoIndex);

        void paint (juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::AudioProcessorValueTreeState& state;
        int index;
        int lastShape = -1;
        float lastPhase = -1.0f;
    };

    class LfoPanel final : public juce::Component
    {
    public:
        LfoPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix, int index);

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        LfoDisplay    display;
        ModSourceChip dragHandle;
        ChoiceBox shape, syncMode, division, trigger;
        ToggleBox bipolar;
        Knob rate, phase, rise, smooth;
    };

    /** The four macro knobs, always visible under the modulator tabs. */
    class MacroStrip final : public juce::Component
    {
    public:
        explicit MacroStrip (juce::AudioProcessorValueTreeState& state);

        void resized() override;

    private:
        std::array<std::unique_ptr<Knob>, ids::numMacros> macros;
        std::array<std::unique_ptr<ModSourceChip>, ids::numMacros> handles;
    };

    /** Drag handles for the sources that are not modules of their own -
        velocity, the keyboard, and the MIDI controllers. */
    class MidiSourcesPanel final : public juce::Component
    {
    public:
        MidiSourcesPanel();

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        std::vector<std::unique_ptr<ModSourceChip>> chips;
        juce::Label help;
    };

    /** Tabbed container holding every envelope and LFO. */
    class ModulatorsPanel final : public juce::Component
    {
    public:
        ModulatorsPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix);

        void resized() override;

    private:
        juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

        std::array<std::unique_ptr<EnvelopePanel>, ids::numEnvelopes> envelopes;
        std::array<std::unique_ptr<LfoPanel>,      ids::numLfos>      lfos;
        MidiSourcesPanel midiSources;
    };
}
