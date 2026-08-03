#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace nog
{
    /**
        Saves, loads and browses presets on disk.

        Presets are the plugin's own XML state written to a per-user folder, not
        the host's program list, which is what lets a patch move between FL
        Studio, a DAW on another machine, and the standalone build.

        The current preset name lives in the APVTS state tree, so a host session
        reopens showing the patch it was saved with.
    */
    class PresetManager
    {
    public:
        static constexpr const char* presetFileExtension = ".nogpreset";
        static constexpr const char* presetNameProperty  = "presetName";

        explicit PresetManager (juce::AudioProcessorValueTreeState& stateToUse);

        /** Where user presets live, created on first use.

            Windows: Documents\FO Studio\NOG Suite\Presets
            macOS:   ~/Documents/FO Studio/NOG Suite/Presets
        */
        static juce::File getUserPresetDirectory();

        /** Writes the current parameter state out under @p name. */
        void save (const juce::String& name);

        /** Loads @p name, doing nothing if the file has gone missing. */
        void load (const juce::String& name);

        void loadNext();
        void loadPrevious();

        /** Resets every parameter to its default - the init patch. */
        void loadDefault();

        void deletePreset (const juce::String& name);

        /** User preset names, sorted, without extensions. Rescans the folder. */
        juce::StringArray getPresetNames() const;

        /** Factory preset names in the order they should be presented. */
        static juce::StringArray getFactoryNames();

        /** Factory names belonging to @p category. */
        static juce::StringArray getFactoryNamesInCategory (const juce::String& category);

        /** True if @p name is one of the built-in patches. A user preset with
            the same name shadows it, since that is what saving over one means. */
        bool isFactoryPreset (const juce::String& name) const;

        /** Every name the browser can offer: factory first, then user. */
        juce::StringArray getAllPresetNames() const;

        juce::String getCurrentPresetName() const;

    private:
        void setCurrentPresetName (const juce::String& name);

        /** Steps through the preset list by @p delta, wrapping at both ends. */
        void step (int delta);

        /** Applies a factory patch: init defaults first, then its overrides. */
        void loadFactory (const juce::String& name);

        juce::AudioProcessorValueTreeState& state;
    };
}
