#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace nog::params
{
    /** Builds the complete host-visible parameter tree.

        Called once, from the AudioProcessor constructor. Everything the host
        can see or automate originates here.
    */
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    /** Choice lists shared between the parameter layout and the editor, so a
        combo box can never disagree with the parameter it drives.           */
    namespace choices
    {
        juce::StringArray waveforms();
        juce::StringArray oscModes();
        juce::StringArray sampleLoopModes();
        juce::StringArray subWaveforms();
        juce::StringArray warpModes();
        juce::StringArray filterTypes();
        juce::StringArray lfoShapes();
        juce::StringArray lfoSyncModes();
        juce::StringArray lfoTriggerModes();
        juce::StringArray voiceModes();
        juce::StringArray glideModes();
        juce::StringArray noiseColours();
        juce::StringArray oversamplingModes();
        juce::StringArray effectTypes();

        /** Tempo divisions for synced LFOs, ordered slowest to fastest. */
        juce::StringArray tempoDivisions();

        /** Length in beats of the tempo division at @p index, for LFO rate. */
        double tempoDivisionInBeats (int index);
    }
}
