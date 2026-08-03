#include "Params/ParameterLayout.h"

#include "DSP/WavetableBank.h"
#include "Modulation/ModDefs.h"
#include "Params/ParameterIDs.h"

namespace nog::params
{
    using Layout    = juce::AudioProcessorValueTreeState::ParameterLayout;
    using FloatAttr = juce::AudioParameterFloatAttributes;
    using Group     = juce::AudioProcessorParameterGroup;

    namespace choices
    {
        juce::StringArray waveforms()
        {
            // The factory wavetables. Reading the names does not build them.
            return dsp::WavetableBank::getTableNames();
        }

        juce::StringArray oscModes()        { return { "Wavetable", "Sample" }; }

        juce::StringArray arpModes()
        {
            return { "Up", "Down", "Up / Down", "Down / Up", "As Played", "Random", "Chord" };
        }
        juce::StringArray sampleLoopModes() { return { "One Shot", "Loop" }; }

        juce::StringArray subWaveforms()   { return { "Sine", "Triangle", "Saw", "Square" }; }

        juce::StringArray warpModes()
        {
            return { "Off", "Sync", "Bend +", "Bend -", "PWM", "Mirror", "Asym", "Quantize" };
        }

        juce::StringArray filterTypes()
        {
            return { "LP 12", "LP 24", "HP 12", "HP 24", "BP 12", "Notch", "Peak", "Allpass" };
        }

        juce::StringArray lfoShapes()
        {
            return { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random S&H", "Random Glide" };
        }

        juce::StringArray lfoSyncModes()    { return { "Free", "Tempo" }; }
        juce::StringArray lfoTriggerModes() { return { "Trigger", "Envelope", "Free Run" }; }
        juce::StringArray voiceModes()      { return { "Poly", "Mono", "Legato" }; }
        juce::StringArray glideModes()      { return { "Off", "Always", "Legato" }; }
        juce::StringArray noiseColours()    { return { "White", "Pink", "Brown" }; }
        juce::StringArray oversamplingModes() { return { "Off", "2x", "4x" }; }

        juce::StringArray effectTypes()
        {
            return { "Bypass", "Distortion", "Bit Crusher", "Chorus", "Flanger",
                     "Phaser", "Delay", "Reverb", "EQ", "Compressor" };
        }

        juce::StringArray tempoDivisions()
        {
            return { "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
        }

        double tempoDivisionInBeats (int index)
        {
            // Beats (quarter notes) per LFO cycle, matching tempoDivisions().
            static constexpr double beats[] { 32.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
            static constexpr int    count = static_cast<int> (std::size (beats));

            return beats[juce::jlimit (0, count - 1, index)];
        }
    }

    // -----------------------------------------------------------------------
    // Formatting + range helpers
    // -----------------------------------------------------------------------
    namespace
    {
        juce::ParameterID pid (const juce::String& id)
        {
            return { id, ids::parameterVersion };
        }

        juce::String fmtPercent (float v, int)   { return juce::String (v * 100.0f, 1) + " %"; }
        juce::String fmtDecibels (float v, int)  { return juce::String (v, 1) + " dB"; }
        juce::String fmtCents (float v, int)     { return juce::String (v, 1) + " ct"; }

        juce::String fmtHertz (float v, int)
        {
            return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                : juce::String (v, v < 10.0f ? 2 : 1) + " Hz";
        }

        juce::String fmtMilliseconds (float v, int)
        {
            return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s"
                                : juce::String (v, v < 10.0f ? 2 : 1) + " ms";
        }

        juce::String fmtPan (float v, int)
        {
            if (std::abs (v) < 0.005f)
                return "C";

            const auto amount = juce::String (std::abs (v) * 100.0f, 0);
            return (v < 0.0f ? "L " : "R ") + amount;
        }

        /** Range whose midpoint sits at @p centre - the usual shape for time
            and frequency controls, where the useful values bunch up low.    */
        juce::NormalisableRange<float> skewed (float min, float max, float centre, float step = 0.0f)
        {
            auto range = juce::NormalisableRange<float> (min, max);
            range.setSkewForCentre (centre);
            range.interval = step;
            return range;
        }

        juce::NormalisableRange<float> linear (float min, float max, float step = 0.0f)
        {
            return { min, max, step };
        }

        std::unique_ptr<juce::AudioParameterFloat> floatParam (const juce::String& id,
                                                               const juce::String& name,
                                                               juce::NormalisableRange<float> range,
                                                               float defaultValue,
                                                               std::function<juce::String (float, int)> formatter = {},
                                                               const juce::String& label = {})
        {
            auto attributes = FloatAttr().withLabel (label);

            if (formatter != nullptr)
                attributes = attributes.withStringFromValueFunction (std::move (formatter));

            return std::make_unique<juce::AudioParameterFloat> (pid (id), name, range,
                                                                defaultValue, attributes);
        }

        std::unique_ptr<juce::AudioParameterChoice> choiceParam (const juce::String& id,
                                                                 const juce::String& name,
                                                                 const juce::StringArray& items,
                                                                 int defaultIndex)
        {
            return std::make_unique<juce::AudioParameterChoice> (pid (id), name, items, defaultIndex);
        }

        std::unique_ptr<juce::AudioParameterBool> boolParam (const juce::String& id,
                                                             const juce::String& name,
                                                             bool defaultValue)
        {
            return std::make_unique<juce::AudioParameterBool> (pid (id), name, defaultValue);
        }

        std::unique_ptr<juce::AudioParameterInt> intParam (const juce::String& id,
                                                           const juce::String& name,
                                                           int min, int max, int defaultValue)
        {
            return std::make_unique<juce::AudioParameterInt> (pid (id), name, min, max, defaultValue);
        }

        std::unique_ptr<Group> group (const juce::String& id, const juce::String& name)
        {
            return std::make_unique<Group> (id, name, "|");
        }

        // ------------------------------------------------------------------
        // Module builders
        // ------------------------------------------------------------------
        std::unique_ptr<Group> buildGlobalGroup()
        {
            auto g = group ("global", "Global");

            g->addChild (floatParam (ids::masterGain, "Master Gain",
                                     skewed (-60.0f, 12.0f, -12.0f), -6.0f, fmtDecibels, "dB"),
                         intParam (ids::polyphony, "Polyphony", 1, 32, 16),
                         choiceParam (ids::voiceMode, "Voice Mode", choices::voiceModes(), 0),
                         choiceParam (ids::glideMode, "Glide Mode", choices::glideModes(), 0),
                         floatParam (ids::glideTime, "Glide Time",
                                     skewed (0.0f, 5000.0f, 250.0f), 0.0f, fmtMilliseconds, "ms"),
                         intParam (ids::pitchBendRange, "Bend Range", 0, 24, 2),
                         floatParam (ids::velocitySens, "Velocity Sens",
                                     linear (0.0f, 1.0f), 0.75f, fmtPercent),
                         choiceParam (ids::oversampling, "Oversampling", choices::oversamplingModes(), 0));

            return g;
        }

        std::unique_ptr<Group> buildArpGroup()
        {
            auto g = group ("arp", "Arpeggiator");

            g->addChild (boolParam   (ids::arpEnable, "Arp On", false),
                         choiceParam (ids::arpMode, "Arp Mode", choices::arpModes(), 0),
                         choiceParam (ids::arpRate, "Arp Rate", choices::tempoDivisions(), 7),
                         intParam    (ids::arpOctaves, "Arp Octaves", 1, 4, 1),
                         floatParam  (ids::arpGate, "Arp Gate", linear (0.05f, 1.0f), 0.5f, fmtPercent),
                         floatParam  (ids::arpSwing, "Arp Swing", linear (0.0f, 1.0f), 0.0f, fmtPercent));

            return g;
        }

        std::unique_ptr<Group> buildOscillatorGroup (int index)
        {
            const auto number = juce::String (index + 1);
            auto g = group ("osc" + number, "Osc " + number);

            // Osc 1 is the one you hear on an init patch; Osc 2 starts silent.
            const auto defaultLevel  = index == 0 ? 0.75f : 0.0f;
            const auto defaultEnable = index == 0;

            g->addChild (boolParam  (ids::osc (index, ids::oscEnable), "Osc " + number + " On", defaultEnable),
                         floatParam (ids::osc (index, ids::oscLevel), "Osc " + number + " Level",
                                     linear (0.0f, 1.0f), defaultLevel, fmtPercent),
                         floatParam (ids::osc (index, ids::oscPan), "Osc " + number + " Pan",
                                     linear (-1.0f, 1.0f), 0.0f, fmtPan),
                         choiceParam (ids::osc (index, ids::oscWave), "Osc " + number + " Wave",
                                      choices::waveforms(), 0),
                         // Halfway through Basic Shapes is a saw, which is the
                         // most useful thing to hear on an init patch.
                         floatParam (ids::osc (index, ids::oscWtPos), "Osc " + number + " WT Pos",
                                     linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         choiceParam (ids::osc (index, ids::oscWarpMode), "Osc " + number + " Warp Mode",
                                      choices::warpModes(), 0),
                         floatParam (ids::osc (index, ids::oscWarpAmount), "Osc " + number + " Warp",
                                     linear (0.0f, 1.0f), 0.0f, fmtPercent),
                         intParam   (ids::osc (index, ids::oscUnison), "Osc " + number + " Unison", 1, 16, 1),
                         floatParam (ids::osc (index, ids::oscDetune), "Osc " + number + " Detune",
                                     linear (0.0f, 1.0f), 0.2f, fmtPercent),
                         floatParam (ids::osc (index, ids::oscBlend), "Osc " + number + " Blend",
                                     linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam (ids::osc (index, ids::oscUniWidth), "Osc " + number + " Uni Width",
                                     linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam (ids::osc (index, ids::oscPhase), "Osc " + number + " Phase",
                                     linear (0.0f, 1.0f), 0.0f, fmtPercent),
                         floatParam (ids::osc (index, ids::oscPhaseRand), "Osc " + number + " Rand Phase",
                                     linear (0.0f, 1.0f), 1.0f, fmtPercent),
                         intParam   (ids::osc (index, ids::oscOctave), "Osc " + number + " Octave", -4, 4, 0),
                         intParam   (ids::osc (index, ids::oscSemi), "Osc " + number + " Semi", -12, 12, 0),
                         floatParam (ids::osc (index, ids::oscFine), "Osc " + number + " Fine",
                                     linear (-100.0f, 100.0f), 0.0f, fmtCents, "ct"),
                         boolParam  (ids::osc (index, ids::oscToFilter), "Osc " + number + " > Filter", true),
                         choiceParam (ids::osc (index, ids::oscMode), "Osc " + number + " Mode",
                                      choices::oscModes(), 0),
                         choiceParam (ids::osc (index, ids::oscSampleLoop), "Osc " + number + " Loop",
                                      choices::sampleLoopModes(), 0),
                         intParam   (ids::osc (index, ids::oscSampleRoot), "Osc " + number + " Root", 0, 127, 60));

            return g;
        }

        std::unique_ptr<Group> buildSubGroup()
        {
            auto g = group ("sub", "Sub");

            g->addChild (boolParam  (ids::subEnable, "Sub On", false),
                         floatParam (ids::subLevel, "Sub Level", linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam (ids::subPan, "Sub Pan", linear (-1.0f, 1.0f), 0.0f, fmtPan),
                         choiceParam (ids::subWave, "Sub Wave", choices::subWaveforms(), 0),
                         intParam   (ids::subOctave, "Sub Octave", -2, 0, -1),
                         boolParam  (ids::subToFilter, "Sub > Filter", false));

            return g;
        }

        std::unique_ptr<Group> buildNoiseGroup()
        {
            auto g = group ("noise", "Noise");

            g->addChild (boolParam  (ids::noiseEnable, "Noise On", false),
                         floatParam (ids::noiseLevel, "Noise Level", linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam (ids::noisePan, "Noise Pan", linear (-1.0f, 1.0f), 0.0f, fmtPan),
                         choiceParam (ids::noiseColour, "Noise Colour", choices::noiseColours(), 0),
                         boolParam  (ids::noiseToFilter, "Noise > Filter", true));

            return g;
        }

        std::unique_ptr<Group> buildFilterGroup()
        {
            auto g = group ("filter", "Filter");

            g->addChild (boolParam  (ids::filterEnable, "Filter On", true),
                         choiceParam (ids::filterType, "Filter Type", choices::filterTypes(), 1),
                         floatParam (ids::filterCutoff, "Cutoff",
                                     skewed (20.0f, 20000.0f, 1000.0f), 20000.0f, fmtHertz, "Hz"),
                         floatParam (ids::filterReso, "Resonance", linear (0.0f, 1.0f), 0.1f, fmtPercent),
                         floatParam (ids::filterDrive, "Drive", linear (0.0f, 1.0f), 0.0f, fmtPercent),
                         floatParam (ids::filterMix, "Filter Mix", linear (0.0f, 1.0f), 1.0f, fmtPercent),
                         floatParam (ids::filterKeytrack, "Key Track", linear (-1.0f, 1.0f), 0.0f, fmtPercent));

            return g;
        }

        std::unique_ptr<Group> buildEnvelopeGroup (int index)
        {
            const auto number = juce::String (index + 1);
            auto g = group ("env" + number, "Env " + number);

            // Env 1 is hard-wired to amplitude, so it opens fully by default.
            const auto defaultSustain = index == 0 ? 1.0f : 0.5f;

            g->addChild (floatParam (ids::env (index, ids::envAttack), "Env " + number + " Attack",
                                     skewed (0.0f, 20000.0f, 200.0f), 2.0f, fmtMilliseconds, "ms"),
                         floatParam (ids::env (index, ids::envHold), "Env " + number + " Hold",
                                     skewed (0.0f, 20000.0f, 200.0f), 0.0f, fmtMilliseconds, "ms"),
                         floatParam (ids::env (index, ids::envDecay), "Env " + number + " Decay",
                                     skewed (0.0f, 20000.0f, 500.0f), 600.0f, fmtMilliseconds, "ms"),
                         floatParam (ids::env (index, ids::envSustain), "Env " + number + " Sustain",
                                     linear (0.0f, 1.0f), defaultSustain, fmtPercent),
                         floatParam (ids::env (index, ids::envRelease), "Env " + number + " Release",
                                     skewed (0.0f, 20000.0f, 500.0f), 120.0f, fmtMilliseconds, "ms"),
                         floatParam (ids::env (index, ids::envAttackCurve), "Env " + number + " Atk Curve",
                                     linear (-1.0f, 1.0f), 0.0f, fmtPercent),
                         floatParam (ids::env (index, ids::envDecayCurve), "Env " + number + " Dec Curve",
                                     linear (-1.0f, 1.0f), 0.0f, fmtPercent),
                         floatParam (ids::env (index, ids::envRelCurve), "Env " + number + " Rel Curve",
                                     linear (-1.0f, 1.0f), 0.0f, fmtPercent));

            return g;
        }

        std::unique_ptr<Group> buildLfoGroup (int index)
        {
            const auto number = juce::String (index + 1);
            auto g = group ("lfo" + number, "LFO " + number);

            g->addChild (choiceParam (ids::lfo (index, ids::lfoShape), "LFO " + number + " Shape",
                                      choices::lfoShapes(), 0),
                         choiceParam (ids::lfo (index, ids::lfoSyncMode), "LFO " + number + " Sync",
                                      choices::lfoSyncModes(), 1),
                         floatParam (ids::lfo (index, ids::lfoRateHz), "LFO " + number + " Rate",
                                     skewed (0.01f, 40.0f, 2.0f), 2.0f, fmtHertz, "Hz"),
                         choiceParam (ids::lfo (index, ids::lfoRateSync), "LFO " + number + " Division",
                                      choices::tempoDivisions(), 5),
                         choiceParam (ids::lfo (index, ids::lfoTrigger), "LFO " + number + " Trigger",
                                      choices::lfoTriggerModes(), 0),
                         floatParam (ids::lfo (index, ids::lfoPhase), "LFO " + number + " Phase",
                                     linear (0.0f, 1.0f), 0.0f, fmtPercent),
                         floatParam (ids::lfo (index, ids::lfoRise), "LFO " + number + " Rise",
                                     skewed (0.0f, 10000.0f, 500.0f), 0.0f, fmtMilliseconds, "ms"),
                         floatParam (ids::lfo (index, ids::lfoSmooth), "LFO " + number + " Smooth",
                                     linear (0.0f, 1.0f), 0.0f, fmtPercent),
                         boolParam  (ids::lfo (index, ids::lfoBipolar), "LFO " + number + " Bipolar", true));

            return g;
        }

        std::unique_ptr<Group> buildMacroGroup()
        {
            auto g = group ("macros", "Macros");

            for (int i = 0; i < ids::numMacros; ++i)
                g->addChild (floatParam (ids::macro (i), "Macro " + juce::String (i + 1),
                                         linear (0.0f, 1.0f), 0.0f, fmtPercent));

            return g;
        }

        std::unique_ptr<Group> buildMatrixGroup()
        {
            auto g = group ("matrix", "Mod Matrix");

            for (int i = 0; i < ids::numMatrixSlots; ++i)
            {
                const auto number = juce::String (i + 1);

                g->addChild (boolParam   (ids::matrix (i, ids::modEnable), "Mod " + number + " On", true),
                             choiceParam (ids::matrix (i, ids::modSource), "Mod " + number + " Source",
                                          mod::sourceChoices(), 0),
                             choiceParam (ids::matrix (i, ids::modDest), "Mod " + number + " Dest",
                                          mod::destChoices(), 0),
                             floatParam  (ids::matrix (i, ids::modAmount), "Mod " + number + " Amount",
                                          linear (-1.0f, 1.0f), 0.0f, fmtPercent),
                             boolParam   (ids::matrix (i, ids::modBipolar), "Mod " + number + " Bipolar", false));
            }

            return g;
        }

        std::unique_ptr<Group> buildFxGroup (int index)
        {
            const auto number = juce::String (index + 1);
            auto g = group ("fx" + number, "FX " + number);

            g->addChild (boolParam   (ids::fx (index, ids::fxEnable), "FX " + number + " On", false),
                         choiceParam (ids::fx (index, ids::fxType), "FX " + number + " Type",
                                      choices::effectTypes(), 0),
                         floatParam  (ids::fx (index, ids::fxMix), "FX " + number + " Mix",
                                      linear (0.0f, 1.0f), 1.0f, fmtPercent),
                         floatParam  (ids::fx (index, ids::fxParamA), "FX " + number + " A",
                                      linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam  (ids::fx (index, ids::fxParamB), "FX " + number + " B",
                                      linear (0.0f, 1.0f), 0.5f, fmtPercent),
                         floatParam  (ids::fx (index, ids::fxParamC), "FX " + number + " C",
                                      linear (0.0f, 1.0f), 0.5f, fmtPercent));

            return g;
        }
    }

    // -----------------------------------------------------------------------
    Layout createLayout()
    {
        Layout layout;

        layout.add (buildGlobalGroup(), buildArpGroup());

        for (int i = 0; i < ids::numOscillators; ++i)
            layout.add (buildOscillatorGroup (i));

        layout.add (buildSubGroup(),
                    buildNoiseGroup(),
                    buildFilterGroup());

        for (int i = 0; i < ids::numEnvelopes; ++i)
            layout.add (buildEnvelopeGroup (i));

        for (int i = 0; i < ids::numLfos; ++i)
            layout.add (buildLfoGroup (i));

        layout.add (buildMacroGroup(),
                    buildMatrixGroup());

        for (int i = 0; i < ids::numFxSlots; ++i)
            layout.add (buildFxGroup (i));

        return layout;
    }
}
