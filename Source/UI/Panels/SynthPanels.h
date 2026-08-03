#pragma once

#include <memory>
#include <vector>

#include "UI/Widgets.h"

namespace nog::ui
{
    /** Lays components out left to right across @p area, splitting it evenly. */
    void layoutRow (juce::Rectangle<int> area, const std::vector<juce::Component*>& components,
                    int gap = 2);

    /** Osc 1 / Osc 2. The two are identical apart from their parameter index. */
    class OscillatorPanel final : public juce::Component
    {
    public:
        OscillatorPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix, int index);

        void resized() override;

    private:
        ToggleBox enable;
        ToggleBox toFilter;
        ChoiceBox wave;
        ChoiceBox warpMode;

        Knob level, pan, wtPos, warp, detune;
        Knob unison, blend, width, phase, phaseRandom;
        Knob octave, semi, fine;
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
}
