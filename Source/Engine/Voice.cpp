#include "Engine/Voice.h"

#include "DSP/WavetableBank.h"
#include "Params/ParameterLayout.h"

namespace nog
{
    namespace
    {
        /** Release time applied when a voice is stolen. Long enough to avoid a
            click, short enough that the new note is not audibly delayed. */
        constexpr float stealReleaseMs = 6.0f;

        constexpr float minimumAudibleLevel = 1.0e-5f;
    }

    void Voice::prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        for (auto& oscillator : oscillators)
            oscillator.prepare (sampleRate);

        subOscillator.prepare (sampleRate);
        filter.prepare (sampleRate);

        for (auto& envelope : envelopes)
            envelope.prepare (sampleRate);

        for (auto& lfo : lfos)
            lfo.prepare (sampleRate);

        reset();
    }

    void Voice::reset()
    {
        active = false;

        for (auto& oscillator : oscillators)
            oscillator.reset();

        subOscillator.reset();
        noise.reset();
        filter.reset();

        for (auto& envelope : envelopes)
            envelope.reset();

        for (auto& lfo : lfos)
            lfo.reset();

        frame.clearOffsets();
        frame.sources.fill (0.0f);
        previousAmplitude = 0.0f;
    }

    void Voice::noteOn (int midiNoteNumber, float noteVelocity, const ParameterStore& parameters,
                        bool retrigger, float glideFromNote)
    {
        midiNote = midiNoteNumber;
        velocity = juce::jlimit (0.0f, 1.0f, noteVelocity);

        targetNote  = static_cast<float> (midiNoteNumber);
        currentNote = glideFromNote > 0.0f ? glideFromNote : targetNote;

        voiceRandom = juce::Random::getSystemRandom().nextFloat();
        stealing    = false;

        // A retriggered voice keeps its filter and oscillator state so that a
        // legato line does not restart the timbre, only the envelopes.
        if (! retrigger)
        {
            for (int i = 0; i < ids::numOscillators; ++i)
            {
                const auto& p = parameters.osc[static_cast<size_t> (i)];

                // The full settings update runs later in the block, but noteOn
                // needs to know now whether it is starting a wavetable or a
                // sample, and where.
                oscillators[static_cast<size_t> (i)].prepareSourceForNoteOn (
                    samples != nullptr ? samples->getSlot (i) : nullptr,
                    p.mode->getIndex(), p.sampleLoop->getIndex(),
                    p.sampleRoot->get(), p.wtPos->get());
            }

            for (auto& oscillator : oscillators)
                oscillator.noteOn();

            subOscillator.noteOn();
            noise.reset();
            filter.reset();
            previousAmplitude = 0.0f;
        }

        for (auto& envelope : envelopes)
            envelope.noteOn();

        for (int i = 0; i < ids::numLfos; ++i)
        {
            const auto mode = static_cast<dsp::Lfo::TriggerMode> (parameters.lfo[static_cast<size_t> (i)].trigger->getIndex());
            lfos[static_cast<size_t> (i)].retrigger (mode);
        }

        updateGlide (0);
        active = true;
    }

    void Voice::noteOff()
    {
        for (auto& envelope : envelopes)
            envelope.noteOff();
    }

    void Voice::steal()
    {
        stealing = true;

        auto settings = dsp::Envelope::Settings {};
        settings.releaseMs = stealReleaseMs;

        // Override only the release: the envelope will ramp from wherever it
        // currently sits down to silence.
        amplitudeEnvelope().setSettings (settings);
        amplitudeEnvelope().noteOff();
    }

    void Voice::retune (int newMidiNote, bool glide) noexcept
    {
        midiNote   = newMidiNote;
        targetNote = static_cast<float> (newMidiNote);

        if (! glide)
            currentNote = targetNote;
    }

    void Voice::updateGlide (int numSamples) noexcept
    {
        if (numSamples <= 0 || glideRate <= 0.0f)
        {
            if (glideRate <= 0.0f)
                currentNote = targetNote;

            return;
        }

        const auto step = glideRate * static_cast<float> (numSamples);

        if (std::abs (targetNote - currentNote) <= step)
            currentNote = targetNote;
        else
            currentNote += std::copysign (step, targetNote - currentNote);
    }

    void Voice::updateModulation (const ParameterStore& parameters, const ModMatrix& matrix,
                                  double bpm, int numSamples)
    {
        // -- envelopes ------------------------------------------------------
        for (int i = 0; i < ids::numEnvelopes; ++i)
        {
            const auto& p = parameters.env[static_cast<size_t> (i)];
            auto& envelope = envelopes[static_cast<size_t> (i)];

            // A stolen voice keeps the shortened release that steal() installed,
            // instead of having the patch's own release time written back over
            // it here - otherwise stealing would take as long as a normal note.
            if (! (i == 0 && stealing))
            {
                dsp::Envelope::Settings settings;
                settings.attackMs     = p.attack->get();
                settings.holdMs       = p.hold->get();
                settings.decayMs      = p.decay->get();
                settings.sustain      = p.sustain->get();
                settings.releaseMs    = p.release->get();
                settings.attackCurve  = p.attackCurve->get();
                settings.decayCurve   = p.decayCurve->get();
                settings.releaseCurve = p.releaseCurve->get();

                envelope.setSettings (settings);
            }

            envelope.getNextValue (numSamples);
        }

        // -- LFOs -----------------------------------------------------------
        for (int i = 0; i < ids::numLfos; ++i)
        {
            const auto& p = parameters.lfo[static_cast<size_t> (i)];
            auto& lfo = lfos[static_cast<size_t> (i)];

            lfo.setShape (static_cast<dsp::Lfo::Shape> (p.shape->getIndex()));
            lfo.setBipolar (p.bipolar->get());
            lfo.setPhaseOffset (p.phase->get());
            lfo.setRiseMs (p.rise->get());
            lfo.setSmoothing (p.smooth->get());

            // Tempo mode resolves the division against the host's BPM here,
            // where the transport is known; the LFO itself only sees Hz.
            const auto tempoSynced = p.syncMode->getIndex() == 1;
            const auto rateDest    = static_cast<mod::Dest> (static_cast<int> (mod::Dest::Lfo1Rate) + i);
            const auto rateOffset  = frame.getOffset (rateDest);

            if (tempoSynced && bpm > 0.0)
            {
                const auto beats = params::choices::tempoDivisionInBeats (p.rateSync->getIndex());
                lfo.setRateHz (static_cast<float> (bpm / 60.0 / juce::jmax (1.0e-6, beats)));
            }
            else
            {
                lfo.setRateHz (parameters.modulated (rateDest, rateOffset));
            }

            lfo.getNextValue (numSamples);
        }

        // -- sample the sources --------------------------------------------
        frame.setSource (mod::Source::Env1, envelopes[0].getValue());
        frame.setSource (mod::Source::Env2, envelopes[1].getValue());
        frame.setSource (mod::Source::Env3, envelopes[2].getValue());

        for (int i = 0; i < ids::numLfos; ++i)
            frame.setSource (static_cast<mod::Source> (static_cast<int> (mod::Source::Lfo1) + i),
                             lfos[static_cast<size_t> (i)].getValue());

        frame.setSource (mod::Source::Velocity,   velocity);
        frame.setSource (mod::Source::KeyTrack,   juce::jlimit (0.0f, 1.0f, currentNote / 127.0f));
        frame.setSource (mod::Source::Random,     voiceRandom);
        frame.setSource (mod::Source::Aftertouch, aftertouch);

        matrix.applyGlobalSources (frame);

        frame.clearOffsets();
        matrix.apply (frame);

        // Pitch bend reaches the oscillators directly rather than through the
        // matrix, because a player expects the wheel to bend the note whether or
        // not a matrix slot has been set up for it.
        pitchBendSemitones = matrix.getPitchBend()
                           * static_cast<float> (parameters.pitchBendRange->get());
    }

    void Voice::applyParameters (const ParameterStore& parameters)
    {
        // Already built: SynthEngine forces the bank during construction, so
        // this is a cheap guarded read rather than a first-use synthesis.
        const auto& bank = dsp::WavetableBank::factory();

        for (int i = 0; i < ids::numOscillators; ++i)
        {
            const auto index = static_cast<size_t> (i);
            const auto& p = parameters.osc[index];
            auto& oscillator = oscillators[index];

            const auto levelDest  = i == 0 ? mod::Dest::Osc1Level  : mod::Dest::Osc2Level;
            const auto panDest    = i == 0 ? mod::Dest::Osc1Pan    : mod::Dest::Osc2Pan;
            const auto wtDest     = i == 0 ? mod::Dest::Osc1WtPos  : mod::Dest::Osc2WtPos;
            const auto warpDest   = i == 0 ? mod::Dest::Osc1Warp   : mod::Dest::Osc2Warp;
            const auto detuneDest = i == 0 ? mod::Dest::Osc1Detune : mod::Dest::Osc2Detune;
            const auto phaseDest  = i == 0 ? mod::Dest::Osc1Phase  : mod::Dest::Osc2Phase;
            const auto pitchDest  = i == 0 ? mod::Dest::Osc1Pitch  : mod::Dest::Osc2Pitch;

            oscillator.setTable (&bank.getTable (p.wave->getIndex()));
            oscillator.setSample (samples != nullptr ? samples->getSlot (i) : nullptr);

            dsp::Oscillator::Settings settings;
            settings.mode         = p.mode->getIndex();
            settings.loop         = p.sampleLoop->getIndex();
            settings.rootNote     = p.sampleRoot->get();
            settings.warpMode     = p.warpMode->getIndex();
            settings.unisonVoices = p.unison->get();
            settings.blend        = p.blend->get();
            settings.width        = p.uniWidth->get();
            settings.phaseRandom  = p.phaseRand->get();

            settings.level      = p.enable->get() ? parameters.modulated (levelDest,  frame.getOffset (levelDest))  : 0.0f;
            settings.pan        = parameters.modulated (panDest,    frame.getOffset (panDest));
            settings.morph      = parameters.modulated (wtDest,     frame.getOffset (wtDest));
            settings.warpAmount = parameters.modulated (warpDest,   frame.getOffset (warpDest));
            settings.detune     = parameters.modulated (detuneDest, frame.getOffset (detuneDest));
            settings.phase      = parameters.modulated (phaseDest,  frame.getOffset (phaseDest));

            oscillator.setSettings (settings);

            // Pitch is a virtual destination: the matrix contributes semitones
            // directly rather than nudging a parameter.
            const auto modulatedSemitones = parameters.modulated (pitchDest, frame.getOffset (pitchDest));

            const auto note = currentNote
                            + static_cast<float> (p.octave->get() * 12)
                            + static_cast<float> (p.semi->get())
                            + p.fine->get() * 0.01f
                            + modulatedSemitones
                            + pitchBendSemitones;

            oscillator.setFrequency (midiNoteToHz (note));
        }

        // -- sub ------------------------------------------------------------
        {
            subOscillator.setTable (&bank.getSubTable());

            dsp::Oscillator::Settings settings;
            settings.unisonVoices = 1;
            settings.phaseRandom  = 0.0f;

            // The sub table holds four exact shapes, one per frame, so the
            // waveform choice addresses a frame directly instead of morphing.
            const auto subFrames = juce::jmax (1, bank.getSubTable().getNumFrames() - 1);
            settings.morph = static_cast<float> (parameters.sub.wave->getIndex())
                           / static_cast<float> (subFrames);
            settings.level = parameters.sub.enable->get()
                           ? parameters.modulated (mod::Dest::SubLevel, frame.getOffset (mod::Dest::SubLevel))
                           : 0.0f;
            settings.pan = parameters.modulated (mod::Dest::SubPan, frame.getOffset (mod::Dest::SubPan));

            subOscillator.setSettings (settings);
            subOscillator.setFrequency (midiNoteToHz (currentNote + static_cast<float> (parameters.sub.octave->get() * 12)));
        }

        noise.setColour (static_cast<dsp::NoiseGenerator::Colour> (parameters.noise.colour->getIndex()));

        // -- filter ---------------------------------------------------------
        {
            const auto cutoff = parameters.modulated (mod::Dest::FilterCutoff, frame.getOffset (mod::Dest::FilterCutoff));
            const auto reso   = parameters.modulated (mod::Dest::FilterReso,   frame.getOffset (mod::Dest::FilterReso));
            const auto drive  = parameters.modulated (mod::Dest::FilterDrive,  frame.getOffset (mod::Dest::FilterDrive));

            // Key tracking shifts the cutoff with the played note so a patch
            // keeps its character across the keyboard.
            const auto keytrack  = parameters.filter.keytrack->get();
            const auto semitones = keytrack * (currentNote - 60.0f);
            const auto tracked   = cutoff * std::exp2 (semitones / 12.0f);

            filter.setParameters (static_cast<dsp::StateVariableFilter::Type> (parameters.filter.type->getIndex()),
                                  tracked, reso, drive);
        }
    }

    void Voice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples,
                                 const ParameterStore& parameters, const ModMatrix& matrix,
                                 double bpm)
    {
        if (! active)
            return;

        auto* left  = output.getWritePointer (0, startSample);
        auto* right = output.getNumChannels() > 1 ? output.getWritePointer (1, startSample) : left;

        // Glide rate is refreshed per block; the mode decides whether the glide
        // parameter applies to this note at all.
        const auto glideMs = parameters.glideTime->get();
        glideRate = glideMs > 0.0f
                  ? 1.0f / (static_cast<float> (sampleRate) * glideMs * 0.001f)
                  : 0.0f;

        const auto masterMix    = parameters.filter.mix->get();
        const auto filterOn     = parameters.filter.enable->get();
        const auto noiseOn      = parameters.noise.enable->get();
        const auto velocitySens = parameters.velocitySens->get();

        // Velocity scales amplitude by a controllable amount rather than
        // absolutely, so a patch can ignore velocity entirely.
        const auto velocityGain = 1.0f - velocitySens + velocitySens * velocity;

        const auto oscToFilter   = std::array { parameters.osc[0].toFilter->get(), parameters.osc[1].toFilter->get() };
        const auto subToFilter   = parameters.sub.toFilter->get();
        const auto noiseToFilter = parameters.noise.toFilter->get();
        const auto noiseLevel    = noiseOn
                                 ? parameters.modulated (mod::Dest::NoiseLevel, frame.getOffset (mod::Dest::NoiseLevel))
                                 : 0.0f;
        const auto noisePan      = parameters.modulated (mod::Dest::NoisePan, frame.getOffset (mod::Dest::NoisePan));
        const auto noiseAngle    = (juce::jlimit (-1.0f, 1.0f, noisePan) + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        const auto noiseLeft     = std::cos (noiseAngle) * juce::MathConstants<float>::sqrt2 * noiseLevel;
        const auto noiseRight    = std::sin (noiseAngle) * juce::MathConstants<float>::sqrt2 * noiseLevel;

        auto samplesRemaining = numSamples;
        auto offset = 0;

        while (samplesRemaining > 0)
        {
            const auto chunk = juce::jmin (modulationBlock, samplesRemaining);

            updateGlide (chunk);
            updateModulation (parameters, matrix, bpm, chunk);
            applyParameters (parameters);

            const auto targetAmplitude = amplitudeEnvelope().getValue() * velocityGain;
            const auto amplitudeStep   = (targetAmplitude - previousAmplitude) / static_cast<float> (chunk);
            auto amplitude = previousAmplitude;

            for (int i = 0; i < chunk; ++i)
            {
                auto filteredLeft = 0.0f, filteredRight = 0.0f;
                auto dryLeft      = 0.0f, dryRight      = 0.0f;

                for (int o = 0; o < ids::numOscillators; ++o)
                {
                    if (oscToFilter[static_cast<size_t> (o)])
                        oscillators[static_cast<size_t> (o)].addNextSample (filteredLeft, filteredRight);
                    else
                        oscillators[static_cast<size_t> (o)].addNextSample (dryLeft, dryRight);
                }

                if (subToFilter)
                    subOscillator.addNextSample (filteredLeft, filteredRight);
                else
                    subOscillator.addNextSample (dryLeft, dryRight);

                if (noiseLevel > 0.0f)
                {
                    const auto sample = noise.getNextValue();

                    if (noiseToFilter)
                    {
                        filteredLeft  += sample * noiseLeft;
                        filteredRight += sample * noiseRight;
                    }
                    else
                    {
                        dryLeft  += sample * noiseLeft;
                        dryRight += sample * noiseRight;
                    }
                }

                if (filterOn)
                {
                    const auto wetLeft  = filter.processSample (0, filteredLeft);
                    const auto wetRight = filter.processSample (1, filteredRight);

                    filteredLeft  += (wetLeft  - filteredLeft)  * masterMix;
                    filteredRight += (wetRight - filteredRight) * masterMix;
                }

                left[offset + i]  += (filteredLeft  + dryLeft)  * amplitude;
                right[offset + i] += (filteredRight + dryRight) * amplitude;

                amplitude += amplitudeStep;
            }

            previousAmplitude = targetAmplitude;
            offset            += chunk;
            samplesRemaining  -= chunk;

            // A voice ends when its amplitude envelope has run out; anything
            // still ringing in the filter is inaudible by then.
            if (! amplitudeEnvelope().isActive() && previousAmplitude < minimumAudibleLevel)
            {
                active = false;
                stealing = false;
                break;
            }
        }
    }
}
