#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

#include "DSP/Envelope.h"
#include "DSP/Lfo.h"
#include "DSP/NoiseGenerator.h"
#include "DSP/Oscillator.h"
#include "DSP/SampleLibrary.h"
#include "DSP/StateVariableFilter.h"
#include "Modulation/ModMatrix.h"
#include "Params/ParameterStore.h"

namespace nog
{
    /**
        A single sounding note.

        The voice runs its modulation at control rate: every `modulationBlock`
        samples it samples its envelopes and LFOs, applies the matrix, and pushes
        the resulting values into the oscillators and filter. Audio in between is
        generated at full rate, with the amplitude interpolated across the
        sub-block so that a fast attack does not step.

        Everything the voice needs from the parameter tree is read through the
        ParameterStore, and everything modulated goes through ModulationFrame -
        no voice ever touches the APVTS or a juce::String.
    */
    class Voice
    {
    public:
        /** Control-rate interval. 32 samples is well under a millisecond at any
            supported sample rate, so modulation stays smooth, while cutting the
            per-sample cost of the matrix by more than an order of magnitude. */
        static constexpr int modulationBlock = 32;

        void prepare (double sampleRate);
        void reset();

        void noteOn (int midiNoteNumber, float velocity, const ParameterStore& parameters,
                     bool retrigger, float glideFromNote);
        void noteOff();

        /** Cuts the voice short for stealing - a very fast release rather than
            an instant stop, which would click. */
        void steal();

        bool isActive() const noexcept    { return active; }
        bool isReleasing() const noexcept { return amplitudeEnvelope().isReleasing(); }
        int  getMidiNote() const noexcept { return midiNote; }
        int  getMidiChannel() const noexcept { return midiChannel; }
        void setMidiChannel (int channel) noexcept { midiChannel = channel; }

        /** Monotonic counter used to find the oldest voice when stealing. */
        juce::uint64 getStartOrder() const noexcept { return startOrder; }
        void setStartOrder (juce::uint64 order) noexcept { startOrder = order; }

        float getCurrentLevel() const noexcept { return amplitudeEnvelope().getValue(); }

        /** Retunes a sounding voice, for mono/legato playing. */
        void retune (int newMidiNote, bool glide) noexcept;

        void setAftertouch (float value) noexcept { aftertouch = value; }

        void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples,
                              const ParameterStore& parameters, const ModMatrix& matrix,
                              double bpm);

        /** Where the oscillators get their samples. Not owned. */
        void setSampleLibrary (const dsp::SampleLibrary* library) noexcept { samples = library; }

    private:
        const dsp::Envelope& amplitudeEnvelope() const noexcept { return envelopes[0]; }
        dsp::Envelope& amplitudeEnvelope() noexcept { return envelopes[0]; }

        /** Samples every source and runs the matrix for this sub-block. */
        void updateModulation (const ParameterStore& parameters, const ModMatrix& matrix,
                               double bpm, int numSamples);

        /** Pushes modulated values into the oscillators, filter and noise. */
        void applyParameters (const ParameterStore& parameters);

        void updateGlide (int numSamples) noexcept;

        static float midiNoteToHz (float note) noexcept
        {
            return 440.0f * std::exp2 ((note - 69.0f) / 12.0f);
        }

        double sampleRate = 44100.0;
        bool   active     = false;

        // Set by steal(), cleared on the next note-on. While set, the amplitude
        // envelope keeps its shortened release instead of being overwritten by
        // the patch's release time on the next control-rate update.
        bool   stealing   = false;

        int   midiNote    = 60;
        int   midiChannel = 1;
        float velocity    = 1.0f;
        float aftertouch  = 0.0f;
        float voiceRandom = 0.0f;

        // Glide state, in MIDI note units so that a slide is perceptually linear.
        float currentNote  = 60.0f;
        float targetNote   = 60.0f;
        float glideRate    = 0.0f;   // notes per sample

        /** Refreshed each control-rate update from the wheel position and the
            global bend range parameter. */
        float pitchBendSemitones = 0.0f;

        juce::uint64 startOrder = 0;

        // Amplitude is interpolated across each sub-block from the previous
        // control-rate value to the new one.
        float previousAmplitude = 0.0f;

        const dsp::SampleLibrary* samples = nullptr;

        std::array<dsp::Oscillator, ids::numOscillators> oscillators;
        dsp::Oscillator      subOscillator;
        dsp::NoiseGenerator  noise;
        dsp::StateVariableFilter filter;

        /** The second filter. Serial puts it after the first, parallel sums
            the two - which is the difference between shaping a sound and
            splitting it into two bands. */
        dsp::StateVariableFilter filter2;

        std::array<dsp::Envelope, ids::numEnvelopes> envelopes;
        std::array<dsp::Lfo,      ids::numLfos>      lfos;

        ModulationFrame frame;
    };
}
