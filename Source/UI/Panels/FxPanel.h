#pragma once

#include <array>
#include <memory>

#include "FX/FXChain.h"
#include "UI/Widgets.h"

namespace nog::ui
{
    /**
        The effects rack: one strip per slot.

        Each slot's three generic controls are relabelled to match the selected
        effect type, so the knobs read "Drive / Tone / Output" on a distortion
        and "Size / Damping / Width" on a reverb without the parameters
        themselves changing.
    */
    class FxPanel final : public juce::Component
    {
    public:
        FxPanel (juce::AudioProcessorValueTreeState& state, const fx::FXChain& chain);

        void resized() override;

    private:
        struct Slot : public juce::Component,
                      private juce::Timer
        {
            Slot (juce::AudioProcessorValueTreeState& state, const fx::FXChain& chain, int index);

            void paint (juce::Graphics& g) override;
            void resized() override;

            ToggleBox enabled;
            ChoiceBox type;
            Knob a, b, c, mix;

            juce::Label title;

        private:
            void timerCallback() override;

            juce::AudioProcessorValueTreeState& state;
            const fx::FXChain& chain;
            int index;
            int lastType = -1;
        };

        std::array<std::unique_ptr<Slot>, ids::numFxSlots> slots;
    };
}
