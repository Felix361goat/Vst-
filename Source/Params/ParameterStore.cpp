#include "Params/ParameterStore.h"

namespace nog
{
    namespace
    {
        /** Resolves a parameter by ID, asserting on the mismatch cases.

            A null here always means the layout and the store disagree - a typo
            in an ID, or a parameter that was declared with a different type.
            That is a programming error, not a runtime condition, so it fires an
            assertion in debug and degrades to a null pointer in release rather
            than taking the host down.
        */
        template <typename ParameterType>
        ParameterType* find (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            auto* parameter = dynamic_cast<ParameterType*> (apvts.getParameter (id));

            // If this fires, check Source/Params/ParameterLayout.cpp declares
            // `id` and declares it with the type being asked for here.
            jassert (parameter != nullptr);

            return parameter;
        }
    }

    void ParameterStore::attach (juce::AudioProcessorValueTreeState& apvts)
    {
        using Float  = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;
        using Bool   = juce::AudioParameterBool;
        using Int    = juce::AudioParameterInt;

        // -- global ---------------------------------------------------------
        masterGain     = find<Float>  (apvts, ids::masterGain);
        polyphony      = find<Int>    (apvts, ids::polyphony);
        voiceMode      = find<Choice> (apvts, ids::voiceMode);
        glideMode      = find<Choice> (apvts, ids::glideMode);
        glideTime      = find<Float>  (apvts, ids::glideTime);
        pitchBendRange = find<Int>    (apvts, ids::pitchBendRange);
        velocitySens   = find<Float>  (apvts, ids::velocitySens);
        oversampling   = find<Choice> (apvts, ids::oversampling);
        limiterEnable  = find<Bool>   (apvts, ids::limiter);

        arpEnable  = find<Bool>   (apvts, ids::arpEnable);
        arpMode    = find<Choice> (apvts, ids::arpMode);
        arpRate    = find<Choice> (apvts, ids::arpRate);
        arpOctaves = find<Int>    (apvts, ids::arpOctaves);
        arpGate    = find<Float>  (apvts, ids::arpGate);
        arpSwing   = find<Float>  (apvts, ids::arpSwing);

        // -- oscillators ----------------------------------------------------
        for (int i = 0; i < ids::numOscillators; ++i)
        {
            auto& o = osc[static_cast<size_t> (i)];

            o.enable     = find<Bool>   (apvts, ids::osc (i, ids::oscEnable));
            o.level      = find<Float>  (apvts, ids::osc (i, ids::oscLevel));
            o.pan        = find<Float>  (apvts, ids::osc (i, ids::oscPan));
            o.wave       = find<Choice> (apvts, ids::osc (i, ids::oscWave));
            o.wtPos      = find<Float>  (apvts, ids::osc (i, ids::oscWtPos));
            o.warpMode   = find<Choice> (apvts, ids::osc (i, ids::oscWarpMode));
            o.warpAmount = find<Float>  (apvts, ids::osc (i, ids::oscWarpAmount));
            o.unison     = find<Int>    (apvts, ids::osc (i, ids::oscUnison));
            o.detune     = find<Float>  (apvts, ids::osc (i, ids::oscDetune));
            o.blend      = find<Float>  (apvts, ids::osc (i, ids::oscBlend));
            o.uniWidth   = find<Float>  (apvts, ids::osc (i, ids::oscUniWidth));
            o.uniSpread  = find<Float>  (apvts, ids::osc (i, ids::oscUniSpread));
            o.phase      = find<Float>  (apvts, ids::osc (i, ids::oscPhase));
            o.phaseRand  = find<Float>  (apvts, ids::osc (i, ids::oscPhaseRand));
            o.octave     = find<Int>    (apvts, ids::osc (i, ids::oscOctave));
            o.semi       = find<Int>    (apvts, ids::osc (i, ids::oscSemi));
            o.fine       = find<Float>  (apvts, ids::osc (i, ids::oscFine));
            o.toFilter   = find<Bool>   (apvts, ids::osc (i, ids::oscToFilter));
            o.mode       = find<Choice> (apvts, ids::osc (i, ids::oscMode));
            o.sampleLoop = find<Choice> (apvts, ids::osc (i, ids::oscSampleLoop));
            o.sampleRoot = find<Int>    (apvts, ids::osc (i, ids::oscSampleRoot));
        }

        // -- sub / noise / filter -------------------------------------------
        sub.enable   = find<Bool>   (apvts, ids::subEnable);
        sub.level    = find<Float>  (apvts, ids::subLevel);
        sub.pan      = find<Float>  (apvts, ids::subPan);
        sub.wave     = find<Choice> (apvts, ids::subWave);
        sub.octave   = find<Int>    (apvts, ids::subOctave);
        sub.toFilter = find<Bool>   (apvts, ids::subToFilter);

        noise.enable   = find<Bool>   (apvts, ids::noiseEnable);
        noise.level    = find<Float>  (apvts, ids::noiseLevel);
        noise.pan      = find<Float>  (apvts, ids::noisePan);
        noise.colour   = find<Choice> (apvts, ids::noiseColour);
        noise.toFilter = find<Bool>   (apvts, ids::noiseToFilter);

        filter.enable   = find<Bool>   (apvts, ids::filterEnable);
        filter.type     = find<Choice> (apvts, ids::filterType);
        filter.cutoff   = find<Float>  (apvts, ids::filterCutoff);
        filter.reso     = find<Float>  (apvts, ids::filterReso);
        filter.drive    = find<Float>  (apvts, ids::filterDrive);
        filter.mix      = find<Float>  (apvts, ids::filterMix);
        filter.keytrack = find<Float>  (apvts, ids::filterKeytrack);

        filter2.enable   = find<Bool>   (apvts, ids::filter2Enable);
        filter2.type     = find<Choice> (apvts, ids::filter2Type);
        filter2.cutoff   = find<Float>  (apvts, ids::filter2Cutoff);
        filter2.reso     = find<Float>  (apvts, ids::filter2Reso);
        filter2.drive    = find<Float>  (apvts, ids::filter2Drive);
        filter2.mix      = find<Float>  (apvts, ids::filter2Mix);
        filter2.keytrack = find<Float>  (apvts, ids::filter2Keytrack);

        filterRouting = find<Choice> (apvts, ids::filterRouting);

        // -- envelopes ------------------------------------------------------
        for (int i = 0; i < ids::numEnvelopes; ++i)
        {
            auto& e = env[static_cast<size_t> (i)];

            e.attack       = find<Float> (apvts, ids::env (i, ids::envAttack));
            e.hold         = find<Float> (apvts, ids::env (i, ids::envHold));
            e.decay        = find<Float> (apvts, ids::env (i, ids::envDecay));
            e.sustain      = find<Float> (apvts, ids::env (i, ids::envSustain));
            e.release      = find<Float> (apvts, ids::env (i, ids::envRelease));
            e.attackCurve  = find<Float> (apvts, ids::env (i, ids::envAttackCurve));
            e.decayCurve   = find<Float> (apvts, ids::env (i, ids::envDecayCurve));
            e.releaseCurve = find<Float> (apvts, ids::env (i, ids::envRelCurve));
        }

        // -- LFOs -----------------------------------------------------------
        for (int i = 0; i < ids::numLfos; ++i)
        {
            auto& l = lfo[static_cast<size_t> (i)];

            l.shape    = find<Choice> (apvts, ids::lfo (i, ids::lfoShape));
            l.syncMode = find<Choice> (apvts, ids::lfo (i, ids::lfoSyncMode));
            l.rateHz   = find<Float>  (apvts, ids::lfo (i, ids::lfoRateHz));
            l.rateSync = find<Choice> (apvts, ids::lfo (i, ids::lfoRateSync));
            l.trigger  = find<Choice> (apvts, ids::lfo (i, ids::lfoTrigger));
            l.phase    = find<Float>  (apvts, ids::lfo (i, ids::lfoPhase));
            l.rise     = find<Float>  (apvts, ids::lfo (i, ids::lfoRise));
            l.smooth   = find<Float>  (apvts, ids::lfo (i, ids::lfoSmooth));
            l.bipolar  = find<Bool>   (apvts, ids::lfo (i, ids::lfoBipolar));
        }

        // -- macros / matrix / FX -------------------------------------------
        for (int i = 0; i < ids::numMacros; ++i)
            macro[static_cast<size_t> (i)] = find<Float> (apvts, ids::macro (i));

        for (int i = 0; i < ids::numMatrixSlots; ++i)
        {
            auto& m = matrix[static_cast<size_t> (i)];

            m.enabled = find<Bool>   (apvts, ids::matrix (i, ids::modEnable));
            m.source  = find<Choice> (apvts, ids::matrix (i, ids::modSource));
            m.dest    = find<Choice> (apvts, ids::matrix (i, ids::modDest));
            m.amount  = find<Float>  (apvts, ids::matrix (i, ids::modAmount));
            m.bipolar = find<Bool>   (apvts, ids::matrix (i, ids::modBipolar));
            m.via     = find<Choice> (apvts, ids::matrix (i, ids::modVia));
            m.curve   = find<Float>  (apvts, ids::matrix (i, ids::modCurve));
        }

        for (int i = 0; i < ids::numFxSlots; ++i)
        {
            auto& f = fx[static_cast<size_t> (i)];

            f.enable = find<Bool>   (apvts, ids::fx (i, ids::fxEnable));
            f.type   = find<Choice> (apvts, ids::fx (i, ids::fxType));
            f.mix    = find<Float>  (apvts, ids::fx (i, ids::fxMix));
            f.a      = find<Float>  (apvts, ids::fx (i, ids::fxParamA));
            f.b      = find<Float>  (apvts, ids::fx (i, ids::fxParamB));
            f.c      = find<Float>  (apvts, ids::fx (i, ids::fxParamC));
        }

        for (int i = 0; i < ids::numMotions; ++i)
        {
            auto& m = motion[static_cast<size_t> (i)];

            m.enable = find<Bool>   (apvts, ids::motion (i, ids::motionEnable));
            m.rate   = find<Choice> (apvts, ids::motion (i, ids::motionRate));
            m.smooth = find<Float>  (apvts, ids::motion (i, ids::motionSmooth));
            m.swing  = find<Float>  (apvts, ids::motion (i, ids::motionSwing));
            m.depth  = find<Float>  (apvts, ids::motion (i, ids::motionDepth));

            for (int step = 0; step < ids::numMotionSteps; ++step)
                m.steps[static_cast<size_t> (step)] = find<Float> (apvts, ids::motionStep (i, step));
        }

        // -- modulation destination table -----------------------------------
        // Virtual destinations (pitch) stay null on purpose; see mod::isVirtual.
        auto setDest = [this] (mod::Dest d, juce::RangedAudioParameter* p)
        {
            destinations[static_cast<size_t> (d)] = p;
        };

        // The effect slots are laid out four destinations at a time in the same
        // order as the slots themselves, so this one is derived rather than
        // spelled out - twenty-four lines of it would be worse, not clearer.
        for (int i = 0; i < ids::numFxSlots; ++i)
        {
            const auto base = static_cast<int> (mod::Dest::Fx1Mix) + i * 4;
            const auto& f = fx[static_cast<size_t> (i)];

            setDest (static_cast<mod::Dest> (base + 0), f.mix);
            setDest (static_cast<mod::Dest> (base + 1), f.a);
            setDest (static_cast<mod::Dest> (base + 2), f.b);
            setDest (static_cast<mod::Dest> (base + 3), f.c);
        }

        static_assert (static_cast<int> (mod::Dest::Fx1Mix) + ids::numFxSlots * 4
                           == static_cast<int> (mod::Dest::Filter2Cutoff),
                       "The effect destinations must stay one contiguous block of four per slot");

        // Spelled out per oscillator rather than derived by arithmetic on the
        // enum, so that reordering mod::Dest cannot silently rewire the matrix.
        struct OscDestinations
        {
            mod::Dest level, pan, wtPos, warp, detune, phase;
        };

        static constexpr OscDestinations oscDestinations[]
        {
            { mod::Dest::Osc1Level, mod::Dest::Osc1Pan, mod::Dest::Osc1WtPos,
              mod::Dest::Osc1Warp,  mod::Dest::Osc1Detune, mod::Dest::Osc1Phase },
            { mod::Dest::Osc2Level, mod::Dest::Osc2Pan, mod::Dest::Osc2WtPos,
              mod::Dest::Osc2Warp,  mod::Dest::Osc2Detune, mod::Dest::Osc2Phase }
        };

        static_assert (std::size (oscDestinations) == static_cast<size_t> (ids::numOscillators),
                       "Add modulation destinations for the new oscillator");

        for (int i = 0; i < ids::numOscillators; ++i)
        {
            const auto& o = osc[static_cast<size_t> (i)];
            const auto& d = oscDestinations[static_cast<size_t> (i)];

            setDest (d.level,  o.level);
            setDest (d.pan,    o.pan);
            setDest (d.wtPos,  o.wtPos);
            setDest (d.warp,   o.warpAmount);
            setDest (d.detune, o.detune);
            setDest (d.phase,  o.phase);
        }

        setDest (mod::Dest::SubLevel,   sub.level);
        setDest (mod::Dest::SubPan,     sub.pan);
        setDest (mod::Dest::NoiseLevel, noise.level);
        setDest (mod::Dest::NoisePan,   noise.pan);

        setDest (mod::Dest::FilterCutoff, filter.cutoff);
        setDest (mod::Dest::FilterReso,   filter.reso);
        setDest (mod::Dest::FilterDrive,  filter.drive);
        setDest (mod::Dest::FilterMix,    filter.mix);

        setDest (mod::Dest::Filter2Cutoff, filter2.cutoff);
        setDest (mod::Dest::Filter2Reso,   filter2.reso);
        setDest (mod::Dest::Filter2Drive,  filter2.drive);
        setDest (mod::Dest::Filter2Mix,    filter2.mix);

        for (int i = 0; i < ids::numLfos; ++i)
        {
            const auto dest = static_cast<mod::Dest> (static_cast<int> (mod::Dest::Lfo1Rate) + i);
            setDest (dest, lfo[static_cast<size_t> (i)].rateHz);
        }

        setDest (mod::Dest::MasterGain, masterGain);
    }

    juce::RangedAudioParameter* ParameterStore::destinationParameter (mod::Dest dest) const noexcept
    {
        const auto index = static_cast<size_t> (dest);

        if (index >= destinations.size())
            return nullptr;

        return destinations[index];
    }

    float ParameterStore::baseValue (mod::Dest dest) const noexcept
    {
        if (auto* parameter = destinationParameter (dest))
            return parameter->convertFrom0to1 (parameter->getValue());

        return 0.0f;
    }

    float ParameterStore::modulated (mod::Dest dest, float normalisedOffset) const noexcept
    {
        // Virtual destinations have no base parameter - the offset *is* the
        // whole value, scaled into the destination's own units.
        if (mod::isVirtual (dest))
            return normalisedOffset * mod::virtualFullScale (dest);

        auto* parameter = destinationParameter (dest);

        if (parameter == nullptr)
            return 0.0f;

        const auto modulated01 = juce::jlimit (0.0f, 1.0f, parameter->getValue() + normalisedOffset);
        return parameter->convertFrom0to1 (modulated01);
    }
}
