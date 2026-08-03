#pragma once

#include <memory>
#include <vector>

#include "UI/Widgets.h"

namespace nog
{
    class NogSuiteProcessor;
}

namespace nog::ui
{
    /** Lays components out left to right across @p area, splitting it evenly. */
    void layoutRow (juce::Rectangle<int> area, const std::vector<juce::Component*>& components,
                    int gap = 2);

    /** Osc 1 / Osc 2. The two are identical apart from their parameter index. */
    class OscillatorPanel final : public juce::Component,
                                  private juce::Timer
    {
    public:
        OscillatorPanel (NogSuiteProcessor& processor, const ModMatrix& matrix, int index);

        void resized() override;

    private:
        void timerCallback() override;

        /** Menu for loading, clearing and configuring the oscillator's sample. */
        void showSampleMenu();
        void promptForSample();

        /** Sample controls only make sense in sample mode, and the wavetable
            selector only in wavetable mode. */
        void updateModeVisibility();

        NogSuiteProcessor& processor;
        int oscillatorIndex;

        ToggleBox enable;
        ToggleBox toFilter;
        ChoiceBox mode;
        ChoiceBox wave;
        ChoiceBox warpMode;

        juce::TextButton sampleButton { "LOAD SAMPLE" };
        std::unique_ptr<juce::FileChooser> fileChooser;
        juce::String lastSampleName;
        int lastMode = -1;

        Knob level, pan, wtPos, warp, detune;
        Knob unison, blend, width, spread, phase, phaseRandom;
        Knob octave, semi, fine;

        WaveDisplay display;
    };

    class SubPanel final : public juce::Component
    {
    public:
        SubPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix);

        void resized() override;

    private:
        ToggleBox enable;
        ToggleBox toFilter;
        ChoiceBox wave;
        Knob level, pan, octave;
    };

    class NoisePanel final : public juce::Component
    {
    public:
        NoisePanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix);

        void resized() override;

    private:
        ToggleBox enable;
        ToggleBox toFilter;
        ChoiceBox colour;
        Knob level, pan;
    };

    class FilterPanel final : public juce::Component
    {
    public:
        FilterPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix);

        void resized() override;

    private:
        ToggleBox enable;
        ChoiceBox type;
        Knob cutoff, resonance, drive, mix, keytrack;
    };

    class GlobalPanel final : public juce::Component
    {
    public:
        explicit GlobalPanel (juce::AudioProcessorValueTreeState& state);

        void resized() override;

    private:
        ChoiceBox voiceMode, glideMode, oversampling;
        Knob polyphony, glideTime, bendRange, velocitySens;
        juce::Label help;
    };

    /** The arpeggiator. Without this the Arp and Sequence patches sound one
        note and stop, which is what they used to do. */
    class ArpPanel final : public juce::Component
    {
    public:
        explicit ArpPanel (juce::AudioProcessorValueTreeState& state);

        void resized() override;

    private:
        ToggleBox enable;
        ChoiceBox mode, rate;
        Knob octaves, gate, swing;
        juce::Label help;
    };
}
