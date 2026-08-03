#pragma once

#include <array>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace nog::presets
{
    /**
        A factory patch: a name, a category, and the parameters that differ from
        the init state.

        Only differences are stored. Loading applies the init defaults first,
        so a preset never has to list the two hundred parameters it does not
        care about, and adding a new parameter later cannot silently change how
        an existing preset sounds - it arrives at its own default.
    */
    struct Preset
    {
        juce::String name;
        juce::String category;
        std::vector<std::pair<juce::String, float>> values;

        /** SampleBank index per oscillator, or -1 for none. Samples are not
            parameters - they live in the state tree beside them - so a patch
            that wants one has to name it separately from its values. */
        std::array<int, 2> builtInSamples { -1, -1 };
    };

    /** Every factory preset, built once on first use. */
    const std::vector<Preset>& all();

    /** Category names in the order they should be presented. */
    juce::StringArray categories();

    /** The preset with this name, or nullptr. */
    const Preset* find (const juce::String& name);
}
