#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

#include "Engine/Voice.h"
#include "FX/FXChain.h"
#include "Modulation/ModMatrix.h"
#include "Params/ParameterStore.h"

namespace nog
{
    /**
        Owns the voice pool, turns MIDI into notes, and mixes the result.

        MIDI is handled sample-accurately: the block is split at every event
        boundary so a note lands on the sample the host asked for rather than at
        the start of the buffer. At small buffer sizes the difference is
        inaudible, but at 2048 samples it is a 45 ms timing error, which is the
        kind of thing that makes a synth feel loose.
    */
    class SynthEngine
    {
    public:
        /** Hard ceiling on simultaneous voices. The polyphony parameter chooses
            how many of these are used; the pool itself is allocated once. */
        static constexpr int maxVoices = 32;

        explicit SynthEngine (ParameterStore& parametersToUse);

        void prepare (double sampleRate, int maximumBlockSize, int numChannels);
        void reset();

        /** Renders one block. @p buffer is expected to be cleared already. */
        void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double bpm);

        int getActiveVoiceCount() const noexcept;

        ModMatrix& getModMatrix() noexcept { return matrix; }

        /** The effects rack, so the editor can ask an effect what its three
            generic controls are called. */
        const fx::FXChain& getEffects() const noexcept { return effects; }

    private:
        enum class VoiceMode { Poly = 0, Mono, Legato };

        void handleMidiMessage (const juce::MidiMessage& message);
        void renderVoices (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double bpm);

        void startNote (int midiNote, float velocity, int channel);
        void stopNote (int midiNote, int channel);
        void allNotesOff (bool allowTailOff);

        /** Finds a voice to use, stealing one if every slot is busy. */
        Voice* allocateVoice();

        VoiceMode getVoiceMode() const noexcept;
        int getPolyphony() const noexcept;

        ParameterStore& parameters;
        ModMatrix       matrix;
        fx::FXChain     effects;

        std::array<Voice, maxVoices> voices;

        double sampleRate = 44100.0;
        juce::uint64 noteCounter = 0;

        // Notes held by the sustain pedal after their key was released.
        std::array<bool, 128> sustainedNotes {};
        bool sustainPedalDown = false;

        /** Order in which keys were pressed, for mono and legato retriggering.
            The newest note is at the end. */
        juce::Array<int> monoNoteStack;
        juce::Array<int> monoVelocityStack;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGain;
    };
}
