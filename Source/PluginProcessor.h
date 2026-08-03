#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine/SynthEngine.h"
#include "Params/ParameterStore.h"
#include "State/PresetManager.h"

namespace nog
{
    /**
        The plugin. Owns the parameter tree, the synth engine and the preset
        manager, and does as little as possible itself.

        The processor is deliberately thin: it converts the host's calls into
        engine calls and handles state. All the synthesis lives in SynthEngine
        and below, which keeps that code testable without a host.
    */
    class NogSuiteProcessor final : public juce::AudioProcessor
    {
    public:
        using APVTS = juce::AudioProcessorValueTreeState;

        /** Bumped when the saved state format changes in a way that needs
            migrating on load. See setStateInformation. */
        static constexpr int currentStateVersion = 1;

        NogSuiteProcessor();
        ~NogSuiteProcessor() override;

        // -- AudioProcessor -------------------------------------------------
        void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
        void releaseResources() override;
        bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        // The engine is single-precision only, so the double-precision overload
        // stays with its base-class behaviour rather than being hidden.
        using AudioProcessor::processBlock;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return JucePlugin_Name; }

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override;

        // A single program: patches are managed by PresetManager on disk rather
        // than through the host's program list.
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return getPresetManager().getCurrentPresetName(); }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock& destData) override;
        void setStateInformation (const void* data, int sizeInBytes) override;

        // -- accessors ------------------------------------------------------
        APVTS&          getValueTreeState() noexcept { return apvts; }
        ParameterStore& getParameterStore() noexcept { return parameters; }
        SynthEngine&    getEngine() noexcept         { return engine; }
        PresetManager&  getPresetManager() noexcept  { return presets; }

        /** Peak output level of the last block, for the editor's meter. */
        float getOutputLevel() const noexcept { return outputLevel.load (std::memory_order_relaxed); }

        int getActiveVoiceCount() const noexcept { return activeVoices.load (std::memory_order_relaxed); }

        /** Editor size, remembered across sessions via the state tree. */
        juce::Point<int> getSavedEditorSize() const;
        void setSavedEditorSize (juce::Point<int> size);

        // -- background artwork ---------------------------------------------
        /** Path of the user's chosen background, or empty for the built-in
            artwork. Falls back to the machine-wide preference when the loaded
            session does not name one, so a chosen background follows the user
            from project to project. */
        juce::String getBackgroundImagePath() const;

        /** Sets the background for this instance and remembers it as the
            default for new ones. An empty path restores the built-in artwork. */
        void setBackgroundImagePath (const juce::String& path);

        /** How far the background is dimmed behind the panels, 0 to 1. Arbitrary
            images vary wildly in brightness, so this is adjustable. */
        float getBackgroundDim() const;
        void setBackgroundDim (float amount);

        /** Where chosen backgrounds are copied to, so that moving or deleting
            the original file does not break the look. */
        static juce::File getBackgroundDirectory();

        /** Copies @p source into the backgrounds folder and selects it.
            Returns false if the file could not be read as an image. */
        bool chooseBackgroundImage (const juce::File& source);

        // -- oscillator samples ----------------------------------------------
        /** Loads an audio file into an oscillator's sample slot. Message thread
            only. Returns false if it could not be decoded. */
        bool loadSampleForOscillator (int oscillatorIndex, const juce::File& file);

        void clearSampleForOscillator (int oscillatorIndex);

        /** Name of the sample loaded into an oscillator, or empty. */
        juce::String getSampleName (int oscillatorIndex) const;

        /** Re-reads the sample paths recorded in the state and loads them.
            Called after a session or preset is restored. */
        void reloadSamplesFromState();

    private:
        static BusesProperties getBusesLayout();

        /** Machine-wide preferences: things that are a property of the user's
            setup rather than of the patch, such as the chosen background. */
        juce::PropertiesFile& getSettings() const;

        APVTS          apvts;
        ParameterStore parameters;
        SynthEngine    engine;
        PresetManager  presets;

        // Written on the audio thread, read by the editor's timer.
        std::atomic<float> outputLevel { 0.0f };
        std::atomic<int>   activeVoices { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NogSuiteProcessor)
    };
}
