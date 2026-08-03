#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Modulation/ModDefs.h"
#include "Params/ParameterIDs.h"

namespace nog
{
    /**
        Typed, cached access to every parameter, for use on the audio thread.

        Looking parameters up by string on the audio thread would mean hashing a
        string per sample block per parameter. Instead every pointer is resolved
        once in attach() and read directly afterwards - each read is a single
        relaxed atomic load, which is what JUCE's parameter classes are built to
        support.

        The store deliberately owns nothing: the AudioProcessorValueTreeState
        outlives it and remains the single source of truth for values and state.
    */
    class ParameterStore
    {
    public:
        struct Oscillator
        {
            juce::AudioParameterBool*   enable      = nullptr;
            juce::AudioParameterFloat*  level       = nullptr;
            juce::AudioParameterFloat*  pan         = nullptr;
            juce::AudioParameterChoice* wave        = nullptr;
            juce::AudioParameterFloat*  wtPos       = nullptr;
            juce::AudioParameterChoice* warpMode    = nullptr;
            juce::AudioParameterFloat*  warpAmount  = nullptr;
            juce::AudioParameterInt*    unison      = nullptr;
            juce::AudioParameterFloat*  detune      = nullptr;
            juce::AudioParameterFloat*  blend       = nullptr;
            juce::AudioParameterFloat*  uniWidth    = nullptr;
            juce::AudioParameterFloat*  phase       = nullptr;
            juce::AudioParameterFloat*  phaseRand   = nullptr;
            juce::AudioParameterInt*    octave      = nullptr;
            juce::AudioParameterInt*    semi        = nullptr;
            juce::AudioParameterFloat*  fine        = nullptr;
            juce::AudioParameterBool*   toFilter    = nullptr;
            juce::AudioParameterChoice* mode        = nullptr;
            juce::AudioParameterChoice* sampleLoop  = nullptr;
            juce::AudioParameterInt*    sampleRoot  = nullptr;
        };

        struct Sub
        {
            juce::AudioParameterBool*   enable   = nullptr;
            juce::AudioParameterFloat*  level    = nullptr;
            juce::AudioParameterFloat*  pan      = nullptr;
            juce::AudioParameterChoice* wave     = nullptr;
            juce::AudioParameterInt*    octave   = nullptr;
            juce::AudioParameterBool*   toFilter = nullptr;
        };

        struct Noise
        {
            juce::AudioParameterBool*   enable   = nullptr;
            juce::AudioParameterFloat*  level    = nullptr;
            juce::AudioParameterFloat*  pan      = nullptr;
            juce::AudioParameterChoice* colour   = nullptr;
            juce::AudioParameterBool*   toFilter = nullptr;
        };

        struct Filter
        {
            juce::AudioParameterBool*   enable   = nullptr;
            juce::AudioParameterChoice* type     = nullptr;
            juce::AudioParameterFloat*  cutoff   = nullptr;
            juce::AudioParameterFloat*  reso     = nullptr;
            juce::AudioParameterFloat*  drive    = nullptr;
            juce::AudioParameterFloat*  mix      = nullptr;
            juce::AudioParameterFloat*  keytrack = nullptr;
        };

        struct Envelope
        {
            juce::AudioParameterFloat* attack       = nullptr;
            juce::AudioParameterFloat* hold         = nullptr;
            juce::AudioParameterFloat* decay        = nullptr;
            juce::AudioParameterFloat* sustain      = nullptr;
            juce::AudioParameterFloat* release      = nullptr;
            juce::AudioParameterFloat* attackCurve  = nullptr;
            juce::AudioParameterFloat* decayCurve   = nullptr;
            juce::AudioParameterFloat* releaseCurve = nullptr;
        };

        struct Lfo
        {
            juce::AudioParameterChoice* shape     = nullptr;
            juce::AudioParameterChoice* syncMode  = nullptr;
            juce::AudioParameterFloat*  rateHz    = nullptr;
            juce::AudioParameterChoice* rateSync  = nullptr;
            juce::AudioParameterChoice* trigger   = nullptr;
            juce::AudioParameterFloat*  phase     = nullptr;
            juce::AudioParameterFloat*  rise      = nullptr;
            juce::AudioParameterFloat*  smooth    = nullptr;
            juce::AudioParameterBool*   bipolar   = nullptr;
        };

        struct ModSlot
        {
            juce::AudioParameterBool*   enabled = nullptr;
            juce::AudioParameterChoice* source  = nullptr;
            juce::AudioParameterChoice* dest    = nullptr;
            juce::AudioParameterFloat*  amount  = nullptr;
            juce::AudioParameterBool*   bipolar = nullptr;
        };

        struct Effect
        {
            juce::AudioParameterBool*   enable = nullptr;
            juce::AudioParameterChoice* type   = nullptr;
            juce::AudioParameterFloat*  mix    = nullptr;
            juce::AudioParameterFloat*  a      = nullptr;
            juce::AudioParameterFloat*  b      = nullptr;
            juce::AudioParameterFloat*  c      = nullptr;
        };

        /** Resolves every pointer. Call once, from the processor constructor,
            after the APVTS has been constructed. */
        void attach (juce::AudioProcessorValueTreeState& apvts);

        /** Applies a normalised modulation offset to a destination's base value
            and returns the result in the destination's own units.

            Modulation is summed in normalised (0..1) parameter space, which is
            what makes a single "amount" knob mean the same proportion of travel
            whether it is driving a 20 Hz..20 kHz cutoff or a -1..1 pan.
        */
        float modulated (mod::Dest dest, float normalisedOffset) const noexcept;

        /** The unmodulated value of a destination, in its own units. */
        float baseValue (mod::Dest dest) const noexcept;

        /** The parameter backing a modulation destination, or nullptr for
            Dest::None. Used by the editor to draw modulation rings.        */
        juce::RangedAudioParameter* destinationParameter (mod::Dest dest) const noexcept;

        // -- global ---------------------------------------------------------
        juce::AudioParameterFloat*  masterGain     = nullptr;
        juce::AudioParameterInt*    polyphony      = nullptr;
        juce::AudioParameterChoice* voiceMode      = nullptr;
        juce::AudioParameterChoice* glideMode      = nullptr;
        juce::AudioParameterFloat*  glideTime      = nullptr;
        juce::AudioParameterInt*    pitchBendRange = nullptr;
        juce::AudioParameterFloat*  velocitySens   = nullptr;
        juce::AudioParameterChoice* oversampling   = nullptr;

        // -- arpeggiator ----------------------------------------------------
        juce::AudioParameterBool*   arpEnable  = nullptr;
        juce::AudioParameterChoice* arpMode    = nullptr;
        juce::AudioParameterChoice* arpRate    = nullptr;
        juce::AudioParameterInt*    arpOctaves = nullptr;
        juce::AudioParameterFloat*  arpGate    = nullptr;
        juce::AudioParameterFloat*  arpSwing   = nullptr;

        // -- modules --------------------------------------------------------
        std::array<Oscillator, ids::numOscillators> osc;
        std::array<Envelope,   ids::numEnvelopes>   env;
        std::array<Lfo,        ids::numLfos>        lfo;
        std::array<ModSlot,    ids::numMatrixSlots> matrix;
        std::array<Effect,     ids::numFxSlots>     fx;
        std::array<juce::AudioParameterFloat*, ids::numMacros> macro {};

        Sub    sub;
        Noise  noise;
        Filter filter;

    private:
        /** Parameter backing each modulation destination, indexed by Dest. */
        std::array<juce::RangedAudioParameter*, mod::numDests> destinations {};
    };
}
