#include "State/FactoryPresets.h"

namespace nog::presets
{
    // Defined in the files under State/Presets. Kept as separate translation
    // units so the library can grow without any one file becoming unreadable.
    std::vector<Preset> bassPresets();
    std::vector<Preset> leadPresets();
    std::vector<Preset> pluckPresets();
    std::vector<Preset> keysPresets();
    std::vector<Preset> guitarPresets();
    std::vector<Preset> bellPresets();
    std::vector<Preset> padPresets();
    std::vector<Preset> sequencePresets();
    std::vector<Preset> y2kPresets();
    std::vector<Preset> effectPresets();
    std::vector<Preset> futureBassPresets();
    std::vector<Preset> trapPresets();
    std::vector<Preset> drillPresets();
    std::vector<Preset> chordPresets();
    std::vector<Preset> arpPresets();
    std::vector<Preset> synthwavePresets();
    std::vector<Preset> orchestralPresets();
    std::vector<Preset> ambientPresets();
    std::vector<Preset> houseTechnoPresets();
    std::vector<Preset> instrumentPresets();
    std::vector<Preset> characterPresets();
    std::vector<Preset> retroPresets();

    juce::StringArray categories()
    {
        // The order the browser presents them in: the things most reached for
        // first, then the more specialised ones.
        return { "Bass", "Lead", "Pluck", "Chords", "Keys", "Plucked",
                 "Bell", "Pad", "Arp", "Sequence",
                 "Future Bass", "Trap", "Drill", "House", "Trance", "Synthwave",
                 "Strings", "Brass", "Chiptune", "Ambient", "FX" };
    }

    const std::vector<Preset>& all()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> combined;

            const auto append = [&combined] (std::vector<Preset> category)
            {
                combined.insert (combined.end(),
                                 std::make_move_iterator (category.begin()),
                                 std::make_move_iterator (category.end()));
            };

            append (instrumentPresets());
            append (characterPresets());
            append (retroPresets());
            append (bassPresets());
            append (leadPresets());
            append (pluckPresets());
            append (keysPresets());
            append (guitarPresets());
            append (bellPresets());
            append (padPresets());
            append (sequencePresets());
            append (chordPresets());
            append (arpPresets());
            append (futureBassPresets());
            append (trapPresets());
            append (drillPresets());
            append (houseTechnoPresets());
            append (y2kPresets());
            append (synthwavePresets());
            append (orchestralPresets());
            append (ambientPresets());
            append (effectPresets());

            return combined;
        }();

        return presets;
    }

    const Preset* find (const juce::String& name)
    {
        for (const auto& preset : all())
            if (preset.name == name)
                return &preset;

        return nullptr;
    }
}
