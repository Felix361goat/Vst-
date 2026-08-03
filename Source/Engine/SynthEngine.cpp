#include "Engine/SynthEngine.h"

namespace nog
{
    SynthEngine::SynthEngine (ParameterStore& parametersToUse)
        : parameters (parametersToUse)
    {
        monoNoteStack.ensureStorageAllocated (128);
        monoVelocityStack.ensureStorageAllocated (128);
    }

    void SynthEngine::prepare (double newSampleRate, int maximumBlockSize, int numChannels)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        for (auto& voice : voices)
            voice.prepare (sampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (1, maximumBlockSize));
        spec.numChannels      = static_cast<juce::uint32> (juce::jmax (1, numChannels));

        effects.prepare (spec);

        masterGain.reset (sampleRate, 0.02);
        masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (parameters.masterGain->get()));
    }

    void SynthEngine::reset()
    {
        for (auto& voice : voices)
            voice.reset();

        effects.reset();
        sustainedNotes.fill (false);
        sustainPedalDown = false;
        monoNoteStack.clearQuick();
        monoVelocityStack.clearQuick();
    }

    SynthEngine::VoiceMode SynthEngine::getVoiceMode() const noexcept
    {
        return static_cast<VoiceMode> (parameters.voiceMode->getIndex());
    }

    int SynthEngine::getPolyphony() const noexcept
    {
        return juce::jlimit (1, maxVoices, parameters.polyphony->get());
    }

    int SynthEngine::getActiveVoiceCount() const noexcept
    {
        auto count = 0;

        for (const auto& voice : voices)
            if (voice.isActive())
                ++count;

        return count;
    }

    Voice* SynthEngine::allocateVoice()
    {
        const auto limit = getPolyphony();

        // A free slot within the polyphony limit is always the best choice.
        for (int i = 0; i < limit; ++i)
            if (! voices[static_cast<size_t> (i)].isActive())
                return &voices[static_cast<size_t> (i)];

        // Otherwise take the quietest voice that is already releasing - stealing
        // a note that is on its way out is the least audible option.
        Voice* quietestReleasing = nullptr;

        for (int i = 0; i < limit; ++i)
        {
            auto& voice = voices[static_cast<size_t> (i)];

            if (! voice.isReleasing())
                continue;

            if (quietestReleasing == nullptr || voice.getCurrentLevel() < quietestReleasing->getCurrentLevel())
                quietestReleasing = &voice;
        }

        if (quietestReleasing != nullptr)
        {
            quietestReleasing->steal();
            return quietestReleasing;
        }

        // Everything is sounding, so take the oldest note.
        Voice* oldest = &voices[0];

        for (int i = 1; i < limit; ++i)
            if (voices[static_cast<size_t> (i)].getStartOrder() < oldest->getStartOrder())
                oldest = &voices[static_cast<size_t> (i)];

        oldest->steal();
        return oldest;
    }

    void SynthEngine::startNote (int midiNote, float velocity, int channel)
    {
        const auto mode = getVoiceMode();

        if (mode != VoiceMode::Poly)
        {
            monoNoteStack.removeAllInstancesOf (midiNote);
            monoNoteStack.add (midiNote);
            monoVelocityStack.add (juce::roundToInt (velocity * 127.0f));

            // In mono and legato the first voice is reused for the whole line.
            auto& voice = voices[0];

            const auto glideMode      = parameters.glideMode->getIndex();  // 0 off, 1 always, 2 legato
            const auto alreadyPlaying = voice.isActive() && ! voice.isReleasing();

            // Legato holds the envelopes open when a second key is added; mono
            // restarts them on every note.
            const auto retrigger = mode == VoiceMode::Legato && alreadyPlaying;
            const auto glide     = glideMode == 1 || (glideMode == 2 && alreadyPlaying);

            if (retrigger)
            {
                voice.retune (midiNote, glide);
            }
            else
            {
                const auto glideFrom = glide && alreadyPlaying ? static_cast<float> (voice.getMidiNote()) : 0.0f;
                voice.setMidiChannel (channel);
                voice.setStartOrder (++noteCounter);
                voice.noteOn (midiNote, velocity, parameters, false, glideFrom);
            }

            return;
        }

        auto* voice = allocateVoice();

        if (voice == nullptr)
            return;

        const auto glideMode = parameters.glideMode->getIndex();
        const auto glideFrom = glideMode == 1 && voice->isActive()
                             ? static_cast<float> (voice->getMidiNote())
                             : 0.0f;

        voice->setMidiChannel (channel);
        voice->setStartOrder (++noteCounter);
        voice->noteOn (midiNote, velocity, parameters, false, glideFrom);
    }

    void SynthEngine::stopNote (int midiNote, int channel)
    {
        // With the pedal down the key release is remembered but not acted on.
        if (sustainPedalDown)
        {
            if (juce::isPositiveAndBelow (midiNote, 128))
                sustainedNotes[static_cast<size_t> (midiNote)] = true;

            return;
        }

        if (getVoiceMode() != VoiceMode::Poly)
        {
            const auto index = monoNoteStack.indexOf (midiNote);

            if (index >= 0)
            {
                monoNoteStack.remove (index);

                if (index < monoVelocityStack.size())
                    monoVelocityStack.remove (index);
            }

            auto& voice = voices[0];

            if (monoNoteStack.isEmpty())
            {
                voice.noteOff();
            }
            else if (voice.isActive())
            {
                // Falling back to the note still being held is what makes a
                // trill work when the keys overlap.
                const auto fallback = monoNoteStack.getLast();
                voice.retune (fallback, parameters.glideMode->getIndex() != 0);
            }

            return;
        }

        for (auto& voice : voices)
            if (voice.isActive() && ! voice.isReleasing()
                && voice.getMidiNote() == midiNote && voice.getMidiChannel() == channel)
                voice.noteOff();
    }

    void SynthEngine::allNotesOff (bool allowTailOff)
    {
        for (auto& voice : voices)
        {
            if (allowTailOff)
                voice.noteOff();
            else
                voice.reset();
        }

        monoNoteStack.clearQuick();
        monoVelocityStack.clearQuick();
        sustainedNotes.fill (false);
    }

    void SynthEngine::handleMidiMessage (const juce::MidiMessage& message)
    {
        if (message.isNoteOn())
        {
            startNote (message.getNoteNumber(), message.getFloatVelocity(), message.getChannel());
        }
        else if (message.isNoteOff())
        {
            stopNote (message.getNoteNumber(), message.getChannel());
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            allNotesOff (message.isAllNotesOff());
        }
        else if (message.isPitchWheel())
        {
            // 0..16383 with 8192 at rest, mapped to -1..1.
            const auto normalised = (static_cast<float> (message.getPitchWheelValue()) - 8192.0f) / 8192.0f;
            matrix.setPitchBend (juce::jlimit (-1.0f, 1.0f, normalised));
        }
        else if (message.isChannelPressure())
        {
            const auto pressure = static_cast<float> (message.getChannelPressureValue()) / 127.0f;
            matrix.setAftertouch (pressure);

            for (auto& voice : voices)
                voice.setAftertouch (pressure);
        }
        else if (message.isController())
        {
            const auto number = message.getControllerNumber();
            const auto value  = static_cast<float> (message.getControllerValue()) / 127.0f;

            if (number == 1)
            {
                matrix.setModWheel (value);
            }
            else if (number == 64)
            {
                sustainPedalDown = message.getControllerValue() >= 64;

                if (! sustainPedalDown)
                {
                    // Releasing the pedal lets go of every key that was already
                    // physically released while it was held.
                    for (int note = 0; note < 128; ++note)
                    {
                        if (! sustainedNotes[static_cast<size_t> (note)])
                            continue;

                        sustainedNotes[static_cast<size_t> (note)] = false;

                        for (auto& voice : voices)
                            if (voice.isActive() && ! voice.isReleasing() && voice.getMidiNote() == note)
                                voice.noteOff();
                    }
                }
            }
        }
    }

    void SynthEngine::renderVoices (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double bpm)
    {
        if (numSamples <= 0)
            return;

        for (auto& voice : voices)
            if (voice.isActive())
                voice.renderNextBlock (buffer, startSample, numSamples, parameters, matrix, bpm);
    }

    void SynthEngine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double bpm)
    {
        matrix.refresh (parameters);

        const auto numSamples = buffer.getNumSamples();
        auto position = 0;

        // Walk the MIDI buffer, rendering the audio between events so that each
        // note starts on exactly the sample the host specified.
        for (const auto metadata : midiMessages)
        {
            const auto eventPosition = juce::jlimit (0, numSamples, metadata.samplePosition);
            const auto segment       = eventPosition - position;

            renderVoices (buffer, position, segment, bpm);
            position += segment;

            handleMidiMessage (metadata.getMessage());
        }

        renderVoices (buffer, position, numSamples - position, bpm);

        effects.process (buffer, parameters);

        masterGain.setTargetValue (juce::Decibels::decibelsToGain (parameters.masterGain->get()));
        masterGain.applyGain (buffer, numSamples);
    }
}
