#include "Engine/SynthEngine.h"

#include "DSP/WavetableBank.h"
#include "Params/ParameterLayout.h"

namespace nog
{
    SynthEngine::SynthEngine (ParameterStore& parametersToUse)
        : parameters (parametersToUse)
    {
        monoNoteStack.ensureStorageAllocated (128);
        monoVelocityStack.ensureStorageAllocated (128);

        // Forces the factory wavetables to be synthesised here, on whatever
        // thread constructs the plugin, rather than on the first audio callback.
        dsp::WavetableBank::factory();

        for (auto& voice : voices)
            voice.setSampleLibrary (&samples);

        arpEvents.reserve (64);
    }

    int SynthEngine::getOversamplingFactor() noexcept
    {
        // Choice order: Off, 2x, 4x.
        switch (parameters.oversampling->getIndex())
        {
            case 1:  return 2;
            case 2:  return 4;
            default: return 1;
        }
    }

    juce::dsp::Oversampling<float>* SynthEngine::getActiveOversampler() noexcept
    {
        const auto choice = parameters.oversampling->getIndex();

        if (choice <= 0 || choice > static_cast<int> (oversamplers.size()))
            return nullptr;

        return oversamplers[static_cast<size_t> (choice - 1)].get();
    }

    int SynthEngine::getLatencySamples() noexcept
    {
        auto latency = 0;

        if (auto* oversampler = getActiveOversampler())
            latency += juce::roundToInt (oversampler->getLatencyInSamples());

        // The limiter looks ahead, so it holds the signal back. Reporting that
        // is what lets the host line the plugin up with everything else; a
        // silently late synth is worse than a slightly delayed one.
        if (parameters.limiterEnable != nullptr && parameters.limiterEnable->get())
            latency += limiter.getLatencySamples();

        return latency;
    }

    Arpeggiator::Settings SynthEngine::getArpSettings() const
    {
        Arpeggiator::Settings settings;
        settings.enabled  = parameters.arpEnable->get();
        settings.mode     = parameters.arpMode->getIndex();
        settings.division = parameters.arpRate->getIndex();
        settings.octaves  = parameters.arpOctaves->get();
        settings.gate     = parameters.arpGate->get();
        settings.swing    = parameters.arpSwing->get();

        return settings;
    }

    void SynthEngine::renderSegment (juce::AudioBuffer<float>& target, int startSample,
                                     int numSamples, double bpm)
    {
        if (numSamples <= 0)
            return;

        const auto settings = getArpSettings();

        if (! settings.enabled && ! arpeggiator.hasHeldNotes())
        {
            renderVoices (target, startSample, numSamples, bpm);
            return;
        }

        // The arp reports what it produced across the segment; each event is a
        // note-on or note-off at a sample offset, and the audio between them is
        // rendered separately so the timing is exact.
        arpEvents.clear();
        arpeggiator.process (numSamples, bpm, settings, arpEvents);

        auto position = 0;

        for (const auto& event : arpEvents)
        {
            const auto offset = juce::jlimit (0, numSamples, event.offset);

            if (offset > position)
            {
                renderVoices (target, startSample + position, offset - position, bpm);
                position = offset;
            }

            if (event.isNoteOn)
                startNote (event.note, event.velocity, 1);
            else
                stopNote (event.note, 1);
        }

        renderVoices (target, startSample + position, numSamples - position, bpm);
    }

    void SynthEngine::prepareVoices()
    {
        // Voices run at the oversampled rate, so everything inside them - the
        // oscillators, the filter, the envelope timing - has to be told about it.
        const auto voiceRate = sampleRate * static_cast<double> (getOversamplingFactor());

        for (auto& voice : voices)
            voice.prepare (voiceRate);

        // The arp runs at the host rate, not the oversampled one: its clock is
        // in real time and the segment lengths it is given are host samples.
        arpeggiator.prepare (sampleRate * static_cast<double> (getOversamplingFactor()));
    }

    void SynthEngine::prepare (double newSampleRate, int maximumBlockSize, int channels)
    {
        sampleRate   = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        maxBlockSize = juce::jmax (1, maximumBlockSize);
        numChannels  = juce::jmax (1, channels);

        // Both factors are built here so that changing the setting later never
        // allocates on the audio thread.
        for (size_t i = 0; i < oversamplers.size(); ++i)
        {
            oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
                static_cast<size_t> (numChannels),
                i + 1,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                true,
                true);

            oversamplers[i]->initProcessing (static_cast<size_t> (maxBlockSize));
        }

        currentOversamplingChoice = parameters.oversampling->getIndex();
        prepareVoices();

        // The effects run at the base rate: the expensive ones gain little from
        // oversampling, and a reverb at 4x would cost four times as much.
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels      = static_cast<juce::uint32> (numChannels);

        effects.prepare (spec);

        limiter.prepare (sampleRate, numChannels);

        for (auto& motion : motions)
            motion.prepare (sampleRate);

        masterGain.reset (sampleRate, 0.02);
        masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (parameters.masterGain->get()));
    }

    void SynthEngine::reset()
    {
        for (auto& voice : voices)
            voice.reset();

        effects.reset();
        arpeggiator.reset();
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
        arpeggiator.reset();
    }

    void SynthEngine::handleMidiMessage (const juce::MidiMessage& message)
    {
        if (message.isNoteOn())
        {
            // With the arp on, a key press feeds the pattern rather than
            // sounding a voice directly.
            if (parameters.arpEnable->get())
                arpeggiator.noteOn (message.getNoteNumber(), message.getFloatVelocity());
            else
                startNote (message.getNoteNumber(), message.getFloatVelocity(), message.getChannel());
        }
        else if (message.isNoteOff())
        {
            arpeggiator.noteOff (message.getNoteNumber());

            if (! parameters.arpEnable->get())
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

    void SynthEngine::renderWithMidi (juce::AudioBuffer<float>& target, juce::MidiBuffer& midiMessages,
                                      double bpm, int factor, int hostNumSamples)
    {
        auto position = 0;

        for (const auto metadata : midiMessages)
        {
            const auto eventPosition = juce::jlimit (0, hostNumSamples, metadata.samplePosition) * factor;
            const auto segment       = eventPosition - position;

            renderSegment (target, position, segment, bpm);
            position += segment;

            handleMidiMessage (metadata.getMessage());
        }

        renderSegment (target, position, target.getNumSamples() - position, bpm);
    }

    void SynthEngine::renderVoicesOversampled (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages, double bpm)
    {
        const auto numSamples = buffer.getNumSamples();
        auto* oversampler = getActiveOversampler();

        if (oversampler == nullptr)
        {
            renderWithMidi (buffer, midiMessages, bpm, 1, numSamples);
            return;
        }

        // The synth has no input, so upsampling a cleared buffer just gives a
        // silent block at the higher rate for the voices to render into.
        const juce::dsp::AudioBlock<const float> inputBlock (buffer);
        auto upsampled = oversampler->processSamplesUp (inputBlock);

        const auto channelsToUse = juce::jmin (static_cast<int> (upsampled.getNumChannels()),
                                               static_cast<int> (buffer.getNumChannels()));

        std::array<float*, 2> channelPointers {};

        for (int channel = 0; channel < channelsToUse && channel < 2; ++channel)
            channelPointers[static_cast<size_t> (channel)] =
                upsampled.getChannelPointer (static_cast<size_t> (channel));

        juce::AudioBuffer<float> upsampledBuffer (channelPointers.data(),
                                                  juce::jmin (channelsToUse, 2),
                                                  static_cast<int> (upsampled.getNumSamples()));

        renderWithMidi (upsampledBuffer, midiMessages, bpm,
                        static_cast<int> (oversampler->getOversamplingFactor()), numSamples);

        juce::dsp::AudioBlock<float> outputBlock (buffer);
        oversampler->processSamplesDown (outputBlock);
    }

    void SynthEngine::updateGlobalModulation (int numSamples, double bpm, double positionInBeats)
    {
        for (int i = 0; i < ids::numMotions; ++i)
        {
            const auto& p = parameters.motion[static_cast<size_t> (i)];

            Motion::Settings settings;
            settings.enabled  = p.enable->get();
            settings.division = p.rate->getIndex();
            settings.smooth   = p.smooth->get();
            settings.swing    = p.swing->get();
            settings.depth    = p.depth->get();

            for (int step = 0; step < ids::numMotionSteps; ++step)
                settings.steps[static_cast<size_t> (step)] = p.steps[static_cast<size_t> (step)]->get();

            const auto beatsPerStep = params::choices::tempoDivisionInBeats (settings.division);
            const auto value = motions[static_cast<size_t> (i)].advance (numSamples, bpm, positionInBeats,
                                                                        settings, beatsPerStep);

            // Published to the matrix so voices see it too, not only the
            // effects rack: a pattern on the filter cutoff is as useful as one
            // on a delay mix.
            matrix.setMotion (i, value);
        }

        // Macros, the MIDI controllers and the patterns themselves, so a
        // routing from any of them into an effect works with no extra plumbing.
        matrix.applyGlobalSources (globalFrame);

        globalFrame.clearOffsets();
        matrix.apply (globalFrame);
    }

    void SynthEngine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
                               double bpm, double positionInBeats)
    {
        matrix.refresh (parameters);

        // Changing the oversampling factor changes the rate every voice runs
        // at, so sounding notes are stopped rather than left running at the
        // wrong speed. Switching this mid-performance is not a normal thing to
        // do, and re-preparing a voice allocates nothing.
        const auto choice = parameters.oversampling->getIndex();

        if (choice != currentOversamplingChoice)
        {
            currentOversamplingChoice = choice;

            for (auto& voice : voices)
                voice.reset();

            if (auto* oversampler = getActiveOversampler())
                oversampler->reset();

            prepareVoices();
        }

        const auto numSamples = buffer.getNumSamples();

        // Before the voices, so a pattern reaches both them and the rack in the
        // same block rather than one being a block behind the other.
        updateGlobalModulation (numSamples, bpm, positionInBeats);

        renderVoicesOversampled (buffer, midiMessages, bpm);

        effects.process (buffer, parameters, globalFrame);

        // Modulated rather than read straight off the parameter, so a step
        // pattern on the master level actually gates - which is the oldest
        // trick in dance music and was not previously possible.
        const auto gainDecibels = parameters.modulated (mod::Dest::MasterGain,
                                                        globalFrame.getOffset (mod::Dest::MasterGain));

        masterGain.setTargetValue (juce::Decibels::decibelsToGain (gainDecibels));
        masterGain.applyGain (buffer, numSamples);

        // After the master gain, so the ceiling means what it says whatever the
        // patch is set to.
        if (parameters.limiterEnable->get())
            limiter.process (buffer);
    }
}
