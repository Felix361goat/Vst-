#include "State/PresetManager.h"

#include "State/FactoryPresets.h"

namespace nog
{
    PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToUse)
        : state (stateToUse)
    {
        // Creating the folder up front means the first save cannot fail because
        // the directory does not exist yet.
        const auto directory = getUserPresetDirectory();

        if (! directory.exists())
            directory.createDirectory();
    }

    juce::File PresetManager::getUserPresetDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile (JucePlugin_Manufacturer)
                   .getChildFile (JucePlugin_Name)
                   .getChildFile ("Presets");
    }

    juce::StringArray PresetManager::getPresetNames() const
    {
        juce::StringArray names;

        const auto directory = getUserPresetDirectory();

        if (! directory.isDirectory())
            return names;

        for (const auto& entry : juce::RangedDirectoryIterator (directory, false,
                                                                juce::String ("*") + presetFileExtension,
                                                                juce::File::findFiles))
            names.add (entry.getFile().getFileNameWithoutExtension());

        names.sortNatural();
        return names;
    }

    juce::StringArray PresetManager::getFactoryNames()
    {
        juce::StringArray names;

        for (const auto& preset : presets::all())
            names.add (preset.name);

        return names;
    }

    juce::StringArray PresetManager::getFactoryNamesInCategory (const juce::String& category)
    {
        juce::StringArray names;

        for (const auto& preset : presets::all())
            if (preset.category == category)
                names.add (preset.name);

        return names;
    }

    bool PresetManager::isFactoryPreset (const juce::String& name) const
    {
        // A saved user preset of the same name wins, so that "save over" does
        // what the user expects rather than being silently ignored.
        if (getPresetNames().contains (name))
            return false;

        return presets::find (name) != nullptr;
    }

    juce::StringArray PresetManager::getAllPresetNames() const
    {
        auto names = getFactoryNames();

        for (const auto& name : getPresetNames())
            if (! names.contains (name))
                names.add (name);

        return names;
    }

    void PresetManager::loadFactory (const juce::String& name)
    {
        const auto* preset = presets::find (name);

        if (preset == nullptr)
            return;

        // Reset first: a preset only stores what differs from the defaults, so
        // without this it would inherit whatever the previous patch left behind.
        // The samples are set below rather than cleared here, so a patch that
        // wants one does not have to load it twice.
        resetParameters();

        for (const auto& [id, value] : preset->values)
        {
            if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id)))
            {
                // Values are written in real units - hertz, milliseconds,
                // semitones - and converted here.
                const auto normalised = parameter->convertTo0to1 (value);
                parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalised));
            }
            else
            {
                jassertfalse;   // preset names a parameter that does not exist
            }
        }

        for (int i = 0; i < numSampleSlots; ++i)
        {
            const auto builtIn = preset->builtInSamples[static_cast<size_t> (i)];

            state.state.setProperty (samplePathProperty (i),
                                     builtIn >= 0 ? juce::var ("builtin:" + juce::String (builtIn))
                                                  : juce::var(),
                                     nullptr);
        }

        notifySampleStateChanged();
        setCurrentPresetName (name);
    }

    juce::String PresetManager::samplePathProperty (int oscillatorIndex)
    {
        return "osc" + juce::String (oscillatorIndex + 1) + "SamplePath";
    }

    void PresetManager::notifySampleStateChanged()
    {
        if (onSampleStateChanged)
            onSampleStateChanged();
    }

    juce::String PresetManager::getCurrentPresetName() const
    {
        return state.state.getProperty (presetNameProperty, "Init").toString();
    }

    void PresetManager::setCurrentPresetName (const juce::String& name)
    {
        state.state.setProperty (presetNameProperty, name, nullptr);
    }

    void PresetManager::save (const juce::String& name)
    {
        const auto trimmed = name.trim();

        if (trimmed.isEmpty())
            return;

        const auto directory = getUserPresetDirectory();

        if (! directory.exists())
            directory.createDirectory();

        // Record the name inside the state so a preset reloaded later still
        // knows what it is called.
        setCurrentPresetName (trimmed);

        const auto file = directory.getChildFile (juce::File::createLegalFileName (trimmed) + presetFileExtension);
        const auto xml  = state.copyState().createXml();

        if (xml != nullptr)
            xml->writeTo (file);
    }

    void PresetManager::load (const juce::String& name)
    {
        if (isFactoryPreset (name))
        {
            loadFactory (name);
            return;
        }

        const auto file = getUserPresetDirectory()
                              .getChildFile (juce::File::createLegalFileName (name) + presetFileExtension);

        if (! file.existsAsFile())
            return;

        const auto xml = juce::XmlDocument::parse (file);

        if (xml == nullptr)
            return;

        auto tree = juce::ValueTree::fromXml (*xml);

        if (! tree.isValid())
            return;

        state.replaceState (tree);

        // A user preset is a whole state tree, so it carries its own sample
        // properties; the engine still has to be told to act on them.
        notifySampleStateChanged();
        setCurrentPresetName (name);
    }

    void PresetManager::loadNext()     { step (1); }
    void PresetManager::loadPrevious() { step (-1); }

    void PresetManager::step (int delta)
    {
        const auto names = getAllPresetNames();

        if (names.isEmpty())
            return;

        const auto current = names.indexOf (getCurrentPresetName());

        // An unsaved patch has no place in the list, so stepping from it starts
        // at the beginning rather than doing nothing.
        const auto next = current < 0 ? 0
                                      : (current + delta + names.size()) % names.size();

        load (names[next]);
    }

    void PresetManager::resetParameters()
    {
        for (auto* parameter : state.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
    }

    void PresetManager::loadDefault()
    {
        resetParameters();

        // An init patch has no sample loaded. Without this the previous patch's
        // sample would stay in the engine and keep sounding.
        for (int i = 0; i < numSampleSlots; ++i)
            state.state.setProperty (samplePathProperty (i), juce::var(), nullptr);

        notifySampleStateChanged();
        setCurrentPresetName ("Init");
    }

    void PresetManager::deletePreset (const juce::String& name)
    {
        const auto file = getUserPresetDirectory()
                              .getChildFile (juce::File::createLegalFileName (name) + presetFileExtension);

        if (file.existsAsFile())
            file.deleteFile();

        if (getCurrentPresetName() == name)
            setCurrentPresetName ("Init");
    }
}
