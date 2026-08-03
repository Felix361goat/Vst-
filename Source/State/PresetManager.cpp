#include "State/PresetManager.h"

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
        setCurrentPresetName (name);
    }

    void PresetManager::loadNext()     { step (1); }
    void PresetManager::loadPrevious() { step (-1); }

    void PresetManager::step (int delta)
    {
        const auto names = getPresetNames();

        if (names.isEmpty())
            return;

        const auto current = names.indexOf (getCurrentPresetName());

        // An unsaved patch has no place in the list, so stepping from it starts
        // at the beginning rather than doing nothing.
        const auto next = current < 0 ? 0
                                      : (current + delta + names.size()) % names.size();

        load (names[next]);
    }

    void PresetManager::loadDefault()
    {
        for (auto* parameter : state.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

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
