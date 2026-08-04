/**
    Unit tests for the parts of NOG Suite that do not need a host.

    These cover the foundations rather than the sound: that every parameter the
    code asks for actually exists, that state survives a save/load round trip,
    that the modulation matrix routes what it says it routes, and that the
    engine produces finite audio and eventually goes quiet. Those are the
    invariants that, if broken, make a plugin crash or hang a DAW.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "Engine/Arpeggiator.h"
#include "Engine/SynthEngine.h"
#include "DSP/Envelope.h"
#include "DSP/Limiter.h"
#include "DSP/StateVariableFilter.h"
#include "Modulation/Motion.h"
#include "DSP/Oscillator.h"
#include "DSP/Wavetable.h"
#include "DSP/WavetableBank.h"
#include "Params/ParameterLayout.h"
#include "Params/ParameterStore.h"
#include <map>

#include "DSP/SampleBank.h"
#include "DSP/SampleLibrary.h"
#include "State/FactoryPresets.h"

namespace
{
    constexpr double testSampleRate = 48000.0;
    constexpr int    testBlockSize  = 512;

    /** A minimal AudioProcessor so the APVTS has something to attach to.

        The real plugin processor cannot be used here: it depends on the JUCE
        plugin client defines, which only exist inside a plugin target.
    */
    class TestProcessor final : public juce::AudioProcessor
    {
    public:
        TestProcessor()
            : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
              apvts (*this, nullptr, juce::Identifier ("NogSuiteState"), nog::params::createLayout()),
              engine (parameters)
        {
            parameters.attach (apvts);
        }

        void prepareToPlay (double sampleRate, int samplesPerBlock) override
        {
            engine.prepare (sampleRate, samplesPerBlock, 2);
        }

        using AudioProcessor::processBlock;

        void releaseResources() override {}
        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
        {
            buffer.clear();
            engine.process (buffer, midi, 120.0);
        }

        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        const juce::String getName() const override { return "NogSuiteTest"; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}

        juce::AudioProcessorValueTreeState apvts;
        nog::ParameterStore parameters;
        nog::SynthEngine    engine;
    };

    /** True if every sample is finite and within a sane range. */
    bool isBufferHealthy (const juce::AudioBuffer<float>& buffer, float limit = 16.0f)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer (channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite (samples[i]) || std::abs (samples[i]) > limit)
                    return false;
        }

        return true;
    }

    // -----------------------------------------------------------------------
    class ParameterTests final : public juce::UnitTest
    {
    public:
        ParameterTests() : UnitTest ("Parameters", "nog") {}

        void runTest() override
        {
            beginTest ("every parameter the store asks for exists");
            {
                // ParameterStore::attach asserts internally on a miss; here the
                // check is explicit so it also fails a release build.
                TestProcessor processor;

                expect (processor.parameters.masterGain != nullptr);
                expect (processor.parameters.polyphony != nullptr);
                expect (processor.parameters.filter.cutoff != nullptr);

                for (int i = 0; i < nog::ids::numOscillators; ++i)
                {
                    const auto& osc = processor.parameters.osc[static_cast<size_t> (i)];
                    expect (osc.level != nullptr, "osc " + juce::String (i) + " level");
                    expect (osc.wave != nullptr,  "osc " + juce::String (i) + " wave");
                    expect (osc.fine != nullptr,  "osc " + juce::String (i) + " fine");
                }

                for (int i = 0; i < nog::ids::numEnvelopes; ++i)
                    expect (processor.parameters.env[static_cast<size_t> (i)].attack != nullptr);

                for (int i = 0; i < nog::ids::numLfos; ++i)
                    expect (processor.parameters.lfo[static_cast<size_t> (i)].rateHz != nullptr);

                for (int i = 0; i < nog::ids::numMatrixSlots; ++i)
                    expect (processor.parameters.matrix[static_cast<size_t> (i)].amount != nullptr);

                for (int i = 0; i < nog::ids::numFxSlots; ++i)
                    expect (processor.parameters.fx[static_cast<size_t> (i)].mix != nullptr);

                for (int i = 0; i < nog::ids::numMacros; ++i)
                    expect (processor.parameters.macro[static_cast<size_t> (i)] != nullptr);
            }

            beginTest ("parameter IDs are unique");
            {
                TestProcessor processor;
                juce::StringArray seen;

                for (auto* parameter : processor.getParameters())
                    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                    {
                        const auto id = ranged->getParameterID();
                        expect (! seen.contains (id), "duplicate parameter ID: " + id);
                        seen.add (id);
                    }

                logMessage ("parameter count: " + juce::String (seen.size()));
                expect (seen.size() > 200, "expected the full parameter surface");
            }

            beginTest ("every modulation destination resolves or is virtual");
            {
                TestProcessor processor;

                for (int i = 1; i < nog::mod::numDests; ++i)
                {
                    const auto dest = static_cast<nog::mod::Dest> (i);
                    const auto resolved = processor.parameters.destinationParameter (dest) != nullptr;

                    expect (resolved || nog::mod::isVirtual (dest),
                            juce::String ("unmapped destination: ") + nog::mod::destNames()[static_cast<size_t> (i)]);
                }
            }

            beginTest ("state survives a save and load round trip");
            {
                TestProcessor processor;

                auto* cutoff = processor.parameters.filter.cutoff;
                cutoff->setValueNotifyingHost (0.31f);

                const auto expected = cutoff->get();
                const auto saved    = processor.apvts.copyState();

                cutoff->setValueNotifyingHost (0.87f);
                processor.apvts.replaceState (saved);

                expectWithinAbsoluteError (cutoff->get(), expected, expected * 1.0e-3f);
            }

            beginTest ("modulation is applied in normalised space and clamped");
            {
                TestProcessor processor;
                const auto& store = processor.parameters;

                auto* cutoff = store.filter.cutoff;
                cutoff->setValueNotifyingHost (0.5f);

                const auto base = store.modulated (nog::mod::Dest::FilterCutoff, 0.0f);
                const auto up   = store.modulated (nog::mod::Dest::FilterCutoff, 0.25f);

                expect (up > base, "positive modulation should raise the cutoff");

                // Anything beyond the ends of the range has to clamp, not wrap.
                const auto ceiling = store.modulated (nog::mod::Dest::FilterCutoff, 10.0f);
                const auto floorValue = store.modulated (nog::mod::Dest::FilterCutoff, -10.0f);

                expectWithinAbsoluteError (ceiling, cutoff->getNormalisableRange().end, 1.0f);
                expectWithinAbsoluteError (floorValue, cutoff->getNormalisableRange().start, 1.0f);

                // Pitch has no backing parameter, so it scales the offset.
                const auto semitones = store.modulated (nog::mod::Dest::Osc1Pitch, 0.5f);
                expectWithinAbsoluteError (semitones, nog::mod::virtualFullScale (nog::mod::Dest::Osc1Pitch) * 0.5f, 0.01f);
            }
        }
    };

    // -----------------------------------------------------------------------
    class ModMatrixTests final : public juce::UnitTest
    {
    public:
        ModMatrixTests() : UnitTest ("Modulation matrix", "nog") {}

        void runTest() override
        {
            beginTest ("an enabled slot routes its source to its destination");
            {
                TestProcessor processor;
                auto& slot = processor.parameters.matrix[0];

                slot.enabled->setValueNotifyingHost (1.0f);
                slot.source->setValueNotifyingHost (
                    slot.source->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro1)));
                slot.dest->setValueNotifyingHost (
                    slot.dest->convertTo0to1 (static_cast<float> (nog::mod::Dest::FilterCutoff)));
                slot.amount->setValueNotifyingHost (slot.amount->convertTo0to1 (0.5f));

                processor.parameters.macro[0]->setValueNotifyingHost (1.0f);

                nog::ModMatrix matrix;
                matrix.refresh (processor.parameters);

                nog::ModulationFrame frame;
                matrix.applyGlobalSources (frame);
                frame.clearOffsets();
                matrix.apply (frame);

                expectWithinAbsoluteError (frame.getOffset (nog::mod::Dest::FilterCutoff), 0.5f, 1.0e-4f);
                expectWithinAbsoluteError (frame.getOffset (nog::mod::Dest::FilterReso), 0.0f, 1.0e-6f);
            }

            beginTest ("a curve bends the source without changing its ends");
            {
                // Every routing was linear, so an envelope could only ever push
                // a destination in a straight line. A curve has to bend the
                // middle while leaving nothing and everything exactly where
                // they were - otherwise it is a depth control with extra steps.
                TestProcessor processor;
                auto& slot = processor.parameters.matrix[0];

                slot.enabled->setValueNotifyingHost (1.0f);
                slot.source->setValueNotifyingHost (
                    slot.source->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro1)));
                slot.dest->setValueNotifyingHost (
                    slot.dest->convertTo0to1 (static_cast<float> (nog::mod::Dest::FilterCutoff)));
                slot.amount->setValueNotifyingHost (slot.amount->convertTo0to1 (1.0f));

                const auto offsetFor = [&processor, &slot] (float source, float curve)
                {
                    slot.curve->setValueNotifyingHost (slot.curve->convertTo0to1 (curve));
                    processor.parameters.macro[0]->setValueNotifyingHost (source);

                    nog::ModMatrix matrix;
                    matrix.refresh (processor.parameters);

                    nog::ModulationFrame frame;
                    matrix.applyGlobalSources (frame);
                    frame.clearOffsets();
                    matrix.apply (frame);

                    return frame.getOffset (nog::mod::Dest::FilterCutoff);
                };

                // The ends are fixed points of the curve whatever it is set to.
                for (const auto curve : { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f })
                {
                    expectWithinAbsoluteError (offsetFor (0.0f, curve), 0.0f, 1.0e-4f);
                    expectWithinAbsoluteError (offsetFor (1.0f, curve), 1.0f, 1.0e-4f);
                }

                // Halfway is where the bend shows.
                // Positive is fast-then-slow and negative is slow-then-fast,
                // matching the envelope stages - which is the whole reason the
                // two share one function.
                const auto linear    = offsetFor (0.5f, 0.0f);
                const auto fastStart = offsetFor (0.5f, 1.0f);
                const auto slowStart = offsetFor (0.5f, -1.0f);

                expectWithinAbsoluteError (linear, 0.5f, 1.0e-3f);

                expect (fastStart > linear + 0.1f,
                        "a positive curve should start quickly, got " + juce::String (fastStart, 4));
                expect (slowStart < linear - 0.1f,
                        "a negative curve should start slowly, got " + juce::String (slowStart, 4));
            }

            beginTest ("a via source scales the slot rather than adding to it");
            {
                // Via is what puts a modulator under the player's control: an
                // LFO to pitch via the mod wheel is vibrato that arrives when
                // it is asked for. It has to scale, not sum, or a closed mod
                // wheel would still let the LFO through.
                TestProcessor processor;
                auto& slot = processor.parameters.matrix[0];

                slot.enabled->setValueNotifyingHost (1.0f);
                slot.source->setValueNotifyingHost (
                    slot.source->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro1)));
                slot.via->setValueNotifyingHost (
                    slot.via->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro2)));
                slot.dest->setValueNotifyingHost (
                    slot.dest->convertTo0to1 (static_cast<float> (nog::mod::Dest::FilterCutoff)));
                slot.amount->setValueNotifyingHost (slot.amount->convertTo0to1 (1.0f));

                processor.parameters.macro[0]->setValueNotifyingHost (1.0f);

                const auto offsetFor = [&processor] (float viaAmount)
                {
                    processor.parameters.macro[1]->setValueNotifyingHost (viaAmount);

                    nog::ModMatrix matrix;
                    matrix.refresh (processor.parameters);

                    nog::ModulationFrame frame;
                    matrix.applyGlobalSources (frame);
                    frame.clearOffsets();
                    matrix.apply (frame);

                    return frame.getOffset (nog::mod::Dest::FilterCutoff);
                };

                expectWithinAbsoluteError (offsetFor (0.0f), 0.0f, 1.0e-4f);
                expectWithinAbsoluteError (offsetFor (0.5f), 0.5f, 1.0e-3f);
                expectWithinAbsoluteError (offsetFor (1.0f), 1.0f, 1.0e-4f);
            }

            beginTest ("a disabled or zero-amount slot contributes nothing");
            {
                TestProcessor processor;
                auto& slot = processor.parameters.matrix[0];

                slot.enabled->setValueNotifyingHost (0.0f);
                slot.source->setValueNotifyingHost (
                    slot.source->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro1)));
                slot.dest->setValueNotifyingHost (
                    slot.dest->convertTo0to1 (static_cast<float> (nog::mod::Dest::FilterCutoff)));
                slot.amount->setValueNotifyingHost (slot.amount->convertTo0to1 (1.0f));

                nog::ModMatrix matrix;
                matrix.refresh (processor.parameters);

                expect (matrix.getRoutings().empty(), "a disabled slot should not be collected");
            }

            beginTest ("bipolar re-centres a unipolar source");
            {
                TestProcessor processor;
                auto& slot = processor.parameters.matrix[0];

                slot.enabled->setValueNotifyingHost (1.0f);
                slot.bipolar->setValueNotifyingHost (1.0f);
                slot.source->setValueNotifyingHost (
                    slot.source->convertTo0to1 (static_cast<float> (nog::mod::Source::Macro1)));
                slot.dest->setValueNotifyingHost (
                    slot.dest->convertTo0to1 (static_cast<float> (nog::mod::Dest::FilterCutoff)));
                slot.amount->setValueNotifyingHost (slot.amount->convertTo0to1 (1.0f));

                // A macro at zero should push fully negative once bipolar.
                processor.parameters.macro[0]->setValueNotifyingHost (0.0f);

                nog::ModMatrix matrix;
                matrix.refresh (processor.parameters);

                nog::ModulationFrame frame;
                matrix.applyGlobalSources (frame);
                frame.clearOffsets();
                matrix.apply (frame);

                expectWithinAbsoluteError (frame.getOffset (nog::mod::Dest::FilterCutoff), -1.0f, 1.0e-4f);
            }
        }
    };

    // -----------------------------------------------------------------------
    class EnvelopeTests final : public juce::UnitTest
    {
    public:
        EnvelopeTests() : UnitTest ("Envelope", "nog") {}

        void runTest() override
        {
            beginTest ("reaches sustain and releases to silence");
            {
                nog::dsp::Envelope envelope;
                envelope.prepare (testSampleRate);

                nog::dsp::Envelope::Settings settings;
                settings.attackMs  = 10.0f;
                settings.holdMs    = 0.0f;
                settings.decayMs   = 10.0f;
                settings.sustain   = 0.5f;
                settings.releaseMs = 10.0f;
                envelope.setSettings (settings);

                envelope.noteOn();

                for (int i = 0; i < static_cast<int> (testSampleRate * 0.1); ++i)
                    envelope.getNextValue();

                expectWithinAbsoluteError (envelope.getValue(), 0.5f, 0.01f);
                expect (envelope.isActive());

                envelope.noteOff();

                for (int i = 0; i < static_cast<int> (testSampleRate * 0.1); ++i)
                    envelope.getNextValue();

                expect (! envelope.isActive(), "the envelope should have finished");
                expectWithinAbsoluteError (envelope.getValue(), 0.0f, 1.0e-4f);
            }

            beginTest ("a zero-length attack does not stall or divide by zero");
            {
                nog::dsp::Envelope envelope;
                envelope.prepare (testSampleRate);

                nog::dsp::Envelope::Settings settings;
                settings.attackMs  = 0.0f;
                settings.decayMs   = 0.0f;
                settings.sustain   = 1.0f;
                envelope.setSettings (settings);

                envelope.noteOn();

                const auto value = envelope.getNextValue();

                expect (std::isfinite (value));
                expectWithinAbsoluteError (value, 1.0f, 1.0e-4f);
            }

            beginTest ("control-rate stepping tracks per-sample stepping");
            {
                // The voice advances envelopes once per sub-block; that must
                // land in the same place as advancing them sample by sample.
                nog::dsp::Envelope perSample, perBlock;
                perSample.prepare (testSampleRate);
                perBlock.prepare (testSampleRate);

                nog::dsp::Envelope::Settings settings;
                settings.attackMs = 100.0f;
                settings.decayMs  = 100.0f;
                settings.sustain  = 0.4f;

                perSample.setSettings (settings);
                perBlock.setSettings (settings);

                perSample.noteOn();
                perBlock.noteOn();

                constexpr int totalSamples = 4800;
                constexpr int step = 32;

                for (int i = 0; i < totalSamples; ++i)
                    perSample.getNextValue();

                for (int i = 0; i < totalSamples / step; ++i)
                    perBlock.getNextValue (step);

                expectWithinAbsoluteError (perBlock.getValue(), perSample.getValue(), 0.02f);
            }
        }
    };

    // -----------------------------------------------------------------------
    class WavetableTests final : public juce::UnitTest
    {
    public:
        WavetableTests() : UnitTest ("Wavetables", "nog") {}

        void runTest() override
        {
            beginTest ("every frame of every mip is normalised to a consistent level");
            {
                // Mip levels are synthesised by separate inverse transforms, and
                // an inverse transform scales by its own length. If that is not
                // compensated, the level jumps every time a note crosses a mip
                // boundary - which is silent in a spectrum plot and glaring in a
                // patch.
                const auto& bank = nog::dsp::WavetableBank::factory();

                for (int index = 0; index < nog::dsp::WavetableBank::getNumTables(); ++index)
                {
                    const auto& table = bank.getTable (index);

                    expect (! table.isEmpty(), table.getName() + " is empty");

                    for (int frame = 0; frame < table.getNumFrames(); ++frame)
                    {
                        for (int mip = 0; mip < nog::dsp::Wavetable::numMipLevels; ++mip)
                        {
                            auto peak = 0.0f;

                            for (int i = 0; i < 1024; ++i)
                                peak = juce::jmax (peak, std::abs (table.getSample (static_cast<float> (frame),
                                                                                    mip, i / 1024.0)));

                            const auto where = table.getName() + " frame " + juce::String (frame)
                                             + " mip " + juce::String (mip);

                            // Only the full-bandwidth mip has to carry signal.
                            // A high mip legitimately goes quiet when a frame's
                            // energy lives entirely in harmonics it has dropped
                            // - an FM frame whose fundamental sits in a Bessel
                            // null, for instance.
                            if (mip == 0)
                                expect (peak > 0.1f, where + " is silent (peak " + juce::String (peak) + ")");

                            expect (peak < 2.0f, where + " is too loud (peak " + juce::String (peak) + ")");
                        }
                    }
                }
            }

            beginTest ("high notes stay free of aliasing");
            {
                // The whole point of the mip pyramid. A naive table read folds
                // every harmonic above Nyquist back into the audible band,
                // landing on frequencies that are not multiples of the note
                // being played - which is what is measured here.
                //
                // The test frequency is an exact multiple of the analysis bin
                // width, so every harmonic lands dead on a bin and no window is
                // needed. That matters: a window's own leakage skirt measures
                // around -47 dB and would swamp the thing being looked for.
                const auto& bank = nog::dsp::WavetableBank::factory();

                constexpr int    fftOrder   = 14;
                constexpr int    fftSize    = 1 << fftOrder;
                constexpr double sampleRate = 48000.0;

                // 797 is prime, which keeps aliases from folding onto harmonics.
                constexpr int    binsPerCycle = 797;
                constexpr float  frequency    = static_cast<float> (binsPerCycle * sampleRate / fftSize);

                juce::dsp::FFT fft (fftOrder);

                for (int index = 0; index < nog::dsp::WavetableBank::getNumTables(); ++index)
                {
                    nog::dsp::Oscillator oscillator;
                    oscillator.prepare (sampleRate);
                    oscillator.setTable (&bank.getTable (index));

                    nog::dsp::Oscillator::Settings settings;
                    settings.level        = 1.0f;
                    settings.morph        = 0.5f;
                    settings.unisonVoices = 1;
                    settings.phaseRandom  = 0.0f;
                    oscillator.setSettings (settings);
                    oscillator.setFrequency (frequency);
                    oscillator.noteOn();

                    std::vector<float> samples (static_cast<size_t> (fftSize) * 2, 0.0f);

                    for (int i = 0; i < fftSize; ++i)
                    {
                        auto left = 0.0f, right = 0.0f;
                        oscillator.addNextSample (left, right);
                        samples[static_cast<size_t> (i)] = left;
                    }

                    fft.performFrequencyOnlyForwardTransform (samples.data());

                    auto harmonicEnergy = 0.0, otherEnergy = 0.0;

                    for (int bin = 1; bin < fftSize / 2; ++bin)
                    {
                        const auto magnitude = static_cast<double> (samples[static_cast<size_t> (bin)]);
                        const auto energy    = magnitude * magnitude;

                        // Exact multiples of the fundamental's bin, allowing one
                        // bin either side for floating point drift.
                        const auto remainder = bin % binsPerCycle;
                        const auto onHarmonic = remainder <= 1 || remainder >= binsPerCycle - 1;

                        if (onHarmonic)
                            harmonicEnergy += energy;
                        else
                            otherEnergy += energy;
                    }

                    const auto decibels = 10.0 * std::log10 ((otherEnergy + 1.0e-30)
                                                             / (harmonicEnergy + 1.0e-30));

                    logMessage (bank.getTable (index).getName()
                                + ": non-harmonic energy " + juce::String (decibels, 1) + " dB");

                    expect (decibels < -70.0,
                            bank.getTable (index).getName() + " aliases: "
                                + juce::String (decibels, 1) + " dB of non-harmonic energy");
                }
            }
        }
    };

    // -----------------------------------------------------------------------
    class OscillatorTests final : public juce::UnitTest
    {
    public:
        OscillatorTests() : UnitTest ("Oscillator", "nog") {}

        void runTest() override
        {
            beginTest ("output stays finite across every table, warp and unison count");
            {
                const auto& bank = nog::dsp::WavetableBank::factory();

                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);

                for (int table = 0; table < nog::dsp::WavetableBank::getNumTables(); ++table)
                {
                    oscillator.setTable (&bank.getTable (table));

                    for (int warp = 0; warp < 8; ++warp)
                    {
                        nog::dsp::Oscillator::Settings settings;
                        settings.warpMode     = warp;
                        settings.warpAmount   = 0.75f;
                        settings.morph        = 0.4f;
                        settings.unisonVoices = 7;
                        settings.detune       = 0.8f;
                        settings.level        = 1.0f;

                        oscillator.setSettings (settings);
                        oscillator.setFrequency (440.0f);
                        oscillator.noteOn();

                        for (int i = 0; i < 2048; ++i)
                        {
                            auto left = 0.0f, right = 0.0f;
                            oscillator.addNextSample (left, right);

                            expect (std::isfinite (left) && std::isfinite (right),
                                    "table " + juce::String (table) + " warp " + juce::String (warp));
                            expect (std::abs (left) < 8.0f && std::abs (right) < 8.0f,
                                    "runaway level on table " + juce::String (table));
                        }
                    }
                }
            }

            beginTest ("frequencies at or above Nyquist are dropped rather than aliased");
            {
                const auto& bank = nog::dsp::WavetableBank::factory();

                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);
                oscillator.setTable (&bank.getTable (0));

                nog::dsp::Oscillator::Settings settings;
                settings.level = 1.0f;
                oscillator.setSettings (settings);

                oscillator.setFrequency (static_cast<float> (testSampleRate));
                oscillator.noteOn();

                auto peak = 0.0f;

                for (int i = 0; i < 512; ++i)
                {
                    auto left = 0.0f, right = 0.0f;
                    oscillator.addNextSample (left, right);
                    peak = juce::jmax (peak, std::abs (left));
                }

                expectWithinAbsoluteError (peak, 0.0f, 1.0e-6f);
            }
        }
    };

    // -----------------------------------------------------------------------
    class SynthEngineTests final : public juce::UnitTest
    {
    public:
        SynthEngineTests() : UnitTest ("Synth engine", "nog") {}

        void runTest() override
        {
            beginTest ("a note produces audio and silence follows its release");
            {
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::MidiBuffer midi;

                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
                processor.processBlock (buffer, midi);

                expect (isBufferHealthy (buffer));
                expect (buffer.getMagnitude (0, testBlockSize) > 1.0e-4f, "the note should be audible");

                // Let it settle, then release and run well past the tail.
                for (int i = 0; i < 20; ++i)
                {
                    midi.clear();
                    processor.processBlock (buffer, midi);
                    expect (isBufferHealthy (buffer));
                }

                midi.clear();
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                processor.processBlock (buffer, midi);

                for (int i = 0; i < 200; ++i)
                {
                    midi.clear();
                    processor.processBlock (buffer, midi);
                }

                expect (processor.engine.getActiveVoiceCount() == 0, "the voice should have ended");
                expectWithinAbsoluteError (buffer.getMagnitude (0, testBlockSize), 0.0f, 1.0e-5f);
            }

            beginTest ("voice count never exceeds the polyphony setting");
            {
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                processor.parameters.polyphony->setValueNotifyingHost (
                    processor.parameters.polyphony->convertTo0to1 (4));

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::MidiBuffer midi;

                for (int note = 40; note < 70; ++note)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, 1.0f), 0);

                processor.processBlock (buffer, midi);

                expect (processor.engine.getActiveVoiceCount() <= 4,
                        "active voices: " + juce::String (processor.engine.getActiveVoiceCount()));
                expect (isBufferHealthy (buffer));
            }

            beginTest ("the textured noise colours play their texture");
            {
                // Three colours are filtered randomness and the rest play a
                // recording. A textured colour that quietly fell back to noise
                // would sound plausible and be wrong, so each one has to be
                // both audible and different from white.
                const auto renderColour = [this] (int colour)
                {
                    TestProcessor processor;

                    auto& noise = processor.parameters.noise;
                    noise.enable->setValueNotifyingHost (1.0f);
                    noise.level->setValueNotifyingHost (1.0f);
                    noise.colour->setValueNotifyingHost (noise.colour->convertTo0to1 (
                        static_cast<float> (colour)));

                    // Oscillators off, so what is measured is the noise layer.
                    for (int i = 0; i < nog::ids::numOscillators; ++i)
                        processor.parameters.osc[static_cast<size_t> (i)]
                            .enable->setValueNotifyingHost (0.0f);

                    processor.prepareToPlay (testSampleRate, testBlockSize);

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                    auto total = 0.0;

                    for (int block = 0; block < 24; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        expect (isBufferHealthy (buffer));

                        for (int i = 0; i < testBlockSize; ++i)
                            total += std::abs (static_cast<double> (buffer.getSample (0, i)));
                    }

                    return total;
                };

                const auto white = renderColour (0);
                expect (white > 1.0, "white noise should be audible");

                const auto names = nog::params::choices::noiseColours();

                // Every textured colour, from Tape onwards.
                for (int colour = 3; colour < names.size(); ++colour)
                {
                    const auto total = renderColour (colour);

                    expect (total > 0.5,
                            names[colour] + " noise is effectively silent (" + juce::String (total, 3) + ")");

                    expect (std::abs (total - white) > 0.01,
                            names[colour] + " noise is indistinguishable from white");
                }
            }

            beginTest ("the two filters differ in serial and in parallel");
            {
                // Serial and parallel are not two ways of saying the same
                // thing: a low-pass then a high-pass leaves a band, while the
                // two summed leaves everything except a band. If the routing
                // switch produced the same audio either way it would be doing
                // nothing.
                const auto renderWith = [this] (bool parallel)
                {
                    TestProcessor processor;

                    auto& one = processor.parameters.filter;
                    one.enable->setValueNotifyingHost (1.0f);
                    one.cutoff->setValueNotifyingHost (one.cutoff->convertTo0to1 (900.0f));

                    auto& two = processor.parameters.filter2;
                    two.enable->setValueNotifyingHost (1.0f);
                    two.type->setValueNotifyingHost (two.type->convertTo0to1 (
                        static_cast<float> (nog::dsp::StateVariableFilter::Type::HighPass24)));
                    two.cutoff->setValueNotifyingHost (two.cutoff->convertTo0to1 (600.0f));

                    processor.parameters.filterRouting->setValueNotifyingHost (parallel ? 1.0f : 0.0f);

                    processor.prepareToPlay (testSampleRate, testBlockSize);

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 45, 1.0f), 0);

                    auto total = 0.0;

                    for (int block = 0; block < 16; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        for (int i = 0; i < testBlockSize; ++i)
                        {
                            expect (std::isfinite (buffer.getSample (0, i)));
                            total += std::abs (static_cast<double> (buffer.getSample (0, i)));
                        }
                    }

                    return total;
                };

                const auto serial   = renderWith (false);
                const auto parallel = renderWith (true);

                expect (serial > 0.0 && parallel > 0.0, "both routings should make sound");

                // Parallel keeps everything either filter passes, so it is the
                // louder of the two by a clear margin at these settings.
                expect (parallel > serial * 1.5,
                        "parallel should pass far more than serial: "
                            + juce::String (parallel, 2) + " against " + juce::String (serial, 2));
            }

            beginTest ("every effect type runs without producing garbage");
            {
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                auto& slot = processor.parameters.fx[0];
                slot.enable->setValueNotifyingHost (1.0f);

                for (int type = 0; type < nog::fx::FXChain::numTypes; ++type)
                {
                    slot.type->setValueNotifyingHost (
                        slot.type->convertTo0to1 (static_cast<float> (type)));

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                    for (int block = 0; block < 8; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        expect (isBufferHealthy (buffer),
                                "effect type " + juce::String (type) + " produced bad samples");
                    }
                }
            }

            beginTest ("a motion pattern lands on the beat the host is at");
            {
                // The clock comes from the playhead rather than being counted
                // locally, so that a pattern stays put when the transport is
                // scrubbed or looped. That only works if the step it reports is
                // a function of the host position, which is what this checks.
                nog::Motion motion;
                motion.prepare (testSampleRate);

                nog::Motion::Settings settings;
                settings.enabled = true;
                settings.depth   = 1.0f;
                settings.smooth  = 0.0f;

                // Alternating full and silent, so the step is unambiguous.
                for (int i = 0; i < nog::ids::numMotionSteps; ++i)
                    settings.steps[static_cast<size_t> (i)] = i % 2 == 0 ? 1.0f : 0.0f;

                constexpr double beatsPerStep = 1.0;

                for (int beat = 0; beat < 8; ++beat)
                {
                    const auto value = motion.advance (64, 120.0, static_cast<double> (beat),
                                                       settings, beatsPerStep);

                    expectWithinAbsoluteError (value, beat % 2 == 0 ? 1.0f : 0.0f, 0.01f);
                }

                // Jumping backwards, as a loop does, must give the same answer
                // rather than continuing from wherever a local counter had got to.
                const auto afterJump = motion.advance (64, 120.0, 2.0, settings, beatsPerStep);
                expectWithinAbsoluteError (afterJump, 1.0f, 0.01f);
            }

            beginTest ("a motion pattern gates the master level");
            {
                // The oldest trick in dance music, and it only works if the
                // master gain is read through the modulation rather than
                // straight off the parameter.
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                auto& motion = processor.parameters.motion[0];
                motion.enable->setValueNotifyingHost (1.0f);
                motion.depth->setValueNotifyingHost (1.0f);
                motion.smooth->setValueNotifyingHost (0.0f);

                // Fully closed everywhere: with the routing set to pull the
                // level down, the output has to collapse.
                for (int i = 0; i < nog::ids::numMotionSteps; ++i)
                    motion.steps[static_cast<size_t> (i)]->setValueNotifyingHost (0.0f);

                auto& route = processor.parameters.matrix[0];
                route.enabled->setValueNotifyingHost (1.0f);
                route.source->setValueNotifyingHost (route.source->convertTo0to1 (
                    static_cast<float> (nog::mod::Source::Motion1)));
                route.dest->setValueNotifyingHost (route.dest->convertTo0to1 (
                    static_cast<float> (nog::mod::Dest::MasterGain)));
                route.amount->setValueNotifyingHost (route.amount->convertTo0to1 (1.0f));
                route.bipolar->setValueNotifyingHost (1.0f);

                const auto peakOf = [&processor]
                {
                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                    auto peak = 0.0f;

                    for (int block = 0; block < 24; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        if (block >= 8)
                            peak = juce::jmax (peak, buffer.getMagnitude (0, testBlockSize));
                    }

                    return peak;
                };

                const auto closed = peakOf();

                // Fully open, same routing: the level comes back.
                for (int i = 0; i < nog::ids::numMotionSteps; ++i)
                    motion.steps[static_cast<size_t> (i)]->setValueNotifyingHost (1.0f);

                processor.engine.reset();
                const auto open = peakOf();

                expect (open > closed * 2.0f,
                        "the pattern should gate the level: open " + juce::String (open, 5)
                            + " against closed " + juce::String (closed, 5));
            }

            beginTest ("a motion pattern drives an effect");
            {
                // The whole reason Motion exists: the effects rack runs once on
                // the summed output, so no per-voice modulator can reach it.
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                auto& slot = processor.parameters.fx[0];
                slot.enable->setValueNotifyingHost (1.0f);
                slot.type->setValueNotifyingHost (slot.type->convertTo0to1 (
                    static_cast<float> (nog::fx::FXChain::Type::BitCrusher)));
                slot.mix->setValueNotifyingHost (0.0f);

                auto& motion = processor.parameters.motion[0];
                motion.enable->setValueNotifyingHost (1.0f);
                motion.depth->setValueNotifyingHost (1.0f);

                for (int i = 0; i < nog::ids::numMotionSteps; ++i)
                    motion.steps[static_cast<size_t> (i)]->setValueNotifyingHost (1.0f);

                auto& route = processor.parameters.matrix[0];
                route.enabled->setValueNotifyingHost (1.0f);
                route.source->setValueNotifyingHost (route.source->convertTo0to1 (
                    static_cast<float> (nog::mod::Source::Motion1)));
                route.dest->setValueNotifyingHost (route.dest->convertTo0to1 (
                    static_cast<float> (nog::mod::Dest::Fx1Mix)));
                route.amount->setValueNotifyingHost (route.amount->convertTo0to1 (1.0f));

                const auto render = [&processor]
                {
                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                    auto total = 0.0;

                    for (int block = 0; block < 16; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        for (int i = 0; i < testBlockSize; ++i)
                            total += std::abs (static_cast<double> (buffer.getSample (0, i)));
                    }

                    return total;
                };

                const auto withMotion = render();

                motion.enable->setValueNotifyingHost (0.0f);
                processor.engine.reset();

                const auto without = render();

                expect (std::abs (withMotion - without) > 1.0,
                        "the pattern should be audible on the effect: "
                            + juce::String (withMotion, 3) + " against " + juce::String (without, 3));
            }

            beginTest ("the limiter holds the ceiling and is transparent below it");
            {
                // The point of it is that nothing gets out above the ceiling,
                // however loud the thing arriving is. The other half matters
                // just as much: a patch at a sensible level must pass through
                // completely untouched, or the limiter is a tone control.
                const auto runThrough = [this] (float amplitude)
                {
                    nog::dsp::Limiter limiter;
                    limiter.prepare (testSampleRate, 2);

                    juce::AudioBuffer<float> buffer (2, 4096);

                    auto highest = 0.0f;

                    for (int block = 0; block < 8; ++block)
                    {
                        for (int i = 0; i < buffer.getNumSamples(); ++i)
                        {
                            const auto phase = static_cast<float> (block * 4096 + i) / 128.0f;
                            const auto value = amplitude * std::sin (phase * juce::MathConstants<float>::twoPi);

                            buffer.setSample (0, i, value);
                            buffer.setSample (1, i, value);
                        }

                        limiter.process (buffer);

                        // Skip the first block: the look-ahead line starts empty,
                        // so its output is the silence it was primed with.
                        if (block == 0)
                            continue;

                        for (int i = 0; i < buffer.getNumSamples(); ++i)
                        {
                            expect (std::isfinite (buffer.getSample (0, i)));
                            highest = juce::jmax (highest, std::abs (buffer.getSample (0, i)));
                        }
                    }

                    return highest;
                };

                // Ten times over the ceiling still comes out at the ceiling.
                const auto loud = runThrough (8.0f);
                expect (loud <= nog::dsp::Limiter::ceiling * 1.02f,
                        "a signal far over the ceiling reached " + juce::String (loud, 4));

                // A quiet signal is passed at exactly its own level.
                const auto quiet = runThrough (0.25f);
                expectWithinAbsoluteError (quiet, 0.25f, 0.002f);
            }

            beginTest ("a transient does not slip past the limiter");
            {
                // Without look-ahead a limiter lets the front of a transient
                // through while its gain is still coming down, which is exactly
                // the part that hurts. The delay line is what prevents that, so
                // it is worth asserting rather than assuming.
                nog::dsp::Limiter limiter;
                limiter.prepare (testSampleRate, 1);

                juce::AudioBuffer<float> buffer (1, 2048);
                buffer.clear();

                // Silence, then a sudden full-scale burst with no ramp at all.
                for (int i = 1024; i < 2048; ++i)
                    buffer.setSample (0, i, i % 2 == 0 ? 6.0f : -6.0f);

                limiter.process (buffer);

                auto highest = 0.0f;

                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    highest = juce::jmax (highest, std::abs (buffer.getSample (0, i)));

                expect (highest <= nog::dsp::Limiter::ceiling * 1.02f,
                        "the transient reached " + juce::String (highest, 4));
            }

            beginTest ("the dimension widener widens and still folds to mono");
            {
                // Two claims worth checking. It has to actually separate the
                // channels, or it is doing nothing; and the centre has to
                // survive being summed to mono, which a plain Haas widener
                // fails - it cancels, and the part disappears on a phone.
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                auto& slot = processor.parameters.fx[0];
                slot.enable->setValueNotifyingHost (1.0f);
                slot.type->setValueNotifyingHost (slot.type->convertTo0to1 (
                    static_cast<float> (nog::fx::FXChain::Type::Dimension)));
                slot.mix->setValueNotifyingHost (1.0f);

                // Mono keep fully up: that is the setting whose whole purpose
                // is surviving the fold, so it is the one worth asserting on.
                slot.c->setValueNotifyingHost (1.0f);

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::MidiBuffer midi;
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                auto separation = 0.0;
                auto monoEnergy = 0.0;
                auto stereoEnergy = 0.0;

                // Skip the first blocks: the delay lines start empty, so the
                // widener has nothing to work with until they fill.
                for (int block = 0; block < 24; ++block)
                {
                    processor.processBlock (buffer, midi);
                    midi.clear();

                    expect (isBufferHealthy (buffer));

                    if (block < 8)
                        continue;

                    for (int i = 0; i < testBlockSize; ++i)
                    {
                        const auto left  = static_cast<double> (buffer.getSample (0, i));
                        const auto right = static_cast<double> (buffer.getSample (1, i));
                        const auto mono  = (left + right) * 0.5;

                        separation   += std::abs (left - right);
                        monoEnergy   += mono * mono;
                        stereoEnergy += (left * left + right * right) * 0.5;
                    }
                }

                expect (separation > 1.0,
                        "the channels should differ, total separation was "
                            + juce::String (separation, 4));

                expect (stereoEnergy > 0.0);
                expect (monoEnergy > stereoEnergy * 0.7,
                        "summing to mono should keep most of the level, kept "
                            + juce::String (monoEnergy / juce::jmax (1.0e-12, stereoEnergy), 3));
            }

            beginTest ("sustain pedal holds notes until it is released");
            {
                TestProcessor processor;
                processor.prepareToPlay (testSampleRate, testBlockSize);

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::MidiBuffer midi;

                midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 1);
                processor.processBlock (buffer, midi);

                midi.clear();
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                processor.processBlock (buffer, midi);

                expect (processor.engine.getActiveVoiceCount() == 1,
                        "the pedal should have held the note");

                midi.clear();
                midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
                processor.processBlock (buffer, midi);

                for (int i = 0; i < 200; ++i)
                {
                    midi.clear();
                    processor.processBlock (buffer, midi);
                }

                expect (processor.engine.getActiveVoiceCount() == 0,
                        "releasing the pedal should end the note");
            }

            beginTest ("survives being run at unusual sample rates and block sizes");
            {
                for (const auto sampleRate : { 22050.0, 44100.0, 96000.0, 192000.0 })
                {
                    for (const auto blockSize : { 1, 7, 64, 2048 })
                    {
                        TestProcessor processor;
                        processor.prepareToPlay (sampleRate, blockSize);

                        juce::AudioBuffer<float> buffer (2, blockSize);
                        juce::MidiBuffer midi;
                        midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 0);

                        for (int block = 0; block < 16; ++block)
                        {
                            processor.processBlock (buffer, midi);
                            midi.clear();

                            expect (isBufferHealthy (buffer),
                                    "rate " + juce::String (sampleRate) + " block " + juce::String (blockSize));
                        }
                    }
                }
            }
        }
    };

    // -----------------------------------------------------------------------
    class FactoryPresetTests final : public juce::UnitTest
    {
    public:
        FactoryPresetTests() : UnitTest ("Factory presets", "nog") {}

        void runTest() override
        {
            const auto& presets = nog::presets::all();

            beginTest ("the library is populated and every name is unique");
            {
                logMessage ("preset count: " + juce::String (presets.size()));
                expect (presets.size() >= 50, "expected a substantial preset library");

                juce::StringArray seen;

                for (const auto& preset : presets)
                {
                    expect (! seen.contains (preset.name), "duplicate preset name: " + preset.name);
                    expect (preset.name.isNotEmpty());
                    expect (preset.category.isNotEmpty(), preset.name + " has no category");
                    seen.add (preset.name);
                }
            }

            beginTest ("every category listed by the browser has presets in it");
            {
                for (const auto& listed : nog::presets::categories())
                {
                    auto count = 0;

                    for (const auto& preset : presets)
                        if (preset.category == listed)
                            ++count;

                    expect (count > 0, "empty category: " + listed);
                }
            }

            beginTest ("every preset names real parameters and stays in range");
            {
                // A typo in a preset's parameter ID would otherwise be silent:
                // the value is simply dropped and the patch quietly sounds wrong.
                TestProcessor processor;

                for (const auto& preset : presets)
                {
                    for (const auto& [id, value] : preset.values)
                    {
                        auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (
                            processor.apvts.getParameter (id));

                        if (parameter == nullptr)
                        {
                            expect (false, preset.name + " sets unknown parameter " + id);
                            continue;
                        }

                        const auto range = parameter->getNormalisableRange();

                        expect (value >= range.start - 1.0e-3f && value <= range.end + 1.0e-3f,
                                preset.name + ": " + id + " = " + juce::String (value)
                                    + " is outside " + juce::String (range.start)
                                    + " to " + juce::String (range.end));
                    }
                }
            }

            beginTest ("every preset makes a healthy, audible sound");
            {
                // The real proof. Each patch is loaded, played, and checked for
                // finite output that is neither silent nor clipping hard.
                for (const auto& preset : presets)
                {
                    TestProcessor processor;
                    apply (processor, preset);
                    processor.prepareToPlay (testSampleRate, testBlockSize);

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);

                    auto peak = 0.0f;

                    // Long enough for slow pads to open up.
                    for (int block = 0; block < 120; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        expect (isBufferHealthy (buffer), preset.name + " produced bad samples");
                        peak = juce::jmax (peak, buffer.getMagnitude (0, testBlockSize));
                    }

                    expect (peak > 1.0e-3f, preset.name + " is silent");
                    expect (peak < 4.0f,
                            preset.name + " is far too loud (peak " + juce::String (peak) + ")");
                }
            }

            beginTest ("instrument presets actually play their sample");
            {
                // An oscillator in sample mode with an empty slot falls back to
                // its wavetable, which sounds plausible and is completely wrong.
                // The only way to catch that is to check the patch sounds
                // different with its sample than without it.
                for (const auto& preset : presets)
                {
                    const auto wantsSample = std::any_of (preset.builtInSamples.begin(),
                                                          preset.builtInSamples.end(),
                                                          [] (int index) { return index >= 0; });

                    if (! wantsSample)
                        continue;

                    const auto withSample    = renderFingerprint (preset, true);
                    const auto withoutSample = renderFingerprint (preset, false);

                    expect (std::abs (withSample - withoutSample) > 1.0e-4,
                            preset.name + " sounds the same with and without its sample, "
                                          "so it is falling back to a wavetable");
                }
            }

            beginTest ("sample presets set their own start offset");
            {
                // In sample mode the morph parameter becomes the playback start
                // offset, and its default is halfway - correct for a wavetable,
                // and for a sample it skips the attack. A patch that inherits
                // that default loses the pick or the hammer, which is the part
                // that makes it sound like an instrument.
                for (const auto& preset : presets)
                {
                    for (int i = 0; i < static_cast<int> (preset.builtInSamples.size()); ++i)
                    {
                        if (preset.builtInSamples[static_cast<size_t> (i)] < 0)
                            continue;

                        const auto id = nog::ids::osc (i, nog::ids::oscWtPos);
                        const auto declared = std::any_of (preset.values.begin(), preset.values.end(),
                                                           [&id] (const auto& pair) { return pair.first == id; });

                        expect (declared,
                                preset.name + " loads a sample on oscillator " + juce::String (i + 1)
                                    + " without setting its start offset");
                    }
                }
            }

            beginTest ("a sample preset renders identically every time");
            {
                // Oscillators start at a random phase by default, which is right
                // for a wavetable and disastrous for a sample: it starts the file
                // at a random point, so a decaying one-shot comes out at a random
                // and usually tiny level. Rendering the same patch twice is the
                // cheapest way to catch that class of bug, because a sample patch
                // that only sometimes sounds is otherwise very hard to pin down.
                for (const auto& preset : presets)
                {
                    // Only the patches whose whole signal path is the sample.
                    // A wavetable oscillator, a sub, a noise layer, a sample-and-
                    // hold LFO and a random arpeggiator are all legitimately
                    // different from one note to the next.
                    const auto sampleOnly = preset.builtInSamples[0] >= 0
                                         && preset.builtInSamples[1] < 0
                                         && ! usesWavetableOscillator (preset, 1)
                                         && ! hasRandomElement (preset);

                    if (! sampleOnly)
                        continue;

                    const auto first  = renderFingerprint (preset, true);
                    const auto second = renderFingerprint (preset, true);

                    expect (std::abs (first - second) < 1.0e-6 * juce::jmax (1.0, std::abs (first)),
                            preset.name + " renders differently each time (" + juce::String (first, 4)
                                + " then " + juce::String (second, 4) + ")");
                }
            }

            beginTest ("every preset releases to silence");
            {
                for (const auto& preset : presets)
                {
                    TestProcessor processor;
                    apply (processor, preset);
                    processor.prepareToPlay (testSampleRate, testBlockSize);

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
                    processor.processBlock (buffer, midi);

                    midi.clear();
                    midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                    processor.processBlock (buffer, midi);

                    // Generous: the longest release in the library is several
                    // seconds, and the effects add a tail on top of that.
                    for (int block = 0; block < 1200; ++block)
                    {
                        midi.clear();
                        processor.processBlock (buffer, midi);
                    }

                    expect (processor.engine.getActiveVoiceCount() == 0,
                            preset.name + " left a voice running");
                }
            }
        }

    private:
        /** True if the preset switches on the given oscillator and leaves it in
            wavetable mode. */
        static bool usesWavetableOscillator (const nog::presets::Preset& preset, int index)
        {
            const auto valueOf = [&preset] (const juce::String& id) -> float
            {
                for (const auto& [name, value] : preset.values)
                    if (name == id)
                        return value;

                return -1.0f;   // not mentioned
            };

            const auto enabled = valueOf (nog::ids::osc (index, nog::ids::oscEnable));
            const auto mode    = valueOf (nog::ids::osc (index, nog::ids::oscMode));

            return enabled > 0.5f && mode < 0.5f;
        }

        /** True if the preset contains something that is meant to differ from
            one note to the next. */
        static bool hasRandomElement (const nog::presets::Preset& preset)
        {
            const auto valueOf = [&preset] (const juce::String& id) -> float
            {
                for (const auto& [name, value] : preset.values)
                    if (name == id)
                        return value;

                return -1.0f;
            };

            if (valueOf (nog::ids::subEnable) > 0.5f || valueOf (nog::ids::noiseEnable) > 0.5f)
                return true;

            // Random and As Played both reorder, but only Random differs run to run.
            if (valueOf (nog::ids::arpEnable) > 0.5f
                && valueOf (nog::ids::arpMode) >= static_cast<float> (nog::Arpeggiator::Mode::Random))
                return true;

            for (int i = 0; i < nog::ids::numLfos; ++i)
                if (valueOf (nog::ids::lfo (i, nog::ids::lfoShape)) >= 5.0f)   // Random S&H upwards
                    return true;

            return false;
        }

        /** Sum of absolute output over a short note, as a cheap stand-in for
            "what this patch sounds like". Two renders that agree to five
            decimal places came from the same signal path. */
        static double renderFingerprint (const nog::presets::Preset& preset, bool loadSamples)
        {
            TestProcessor processor;
            apply (processor, preset, loadSamples);
            processor.prepareToPlay (testSampleRate, testBlockSize);

            juce::AudioBuffer<float> buffer (2, testBlockSize);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);

            auto total = 0.0;

            for (int block = 0; block < 40; ++block)
            {
                processor.processBlock (buffer, midi);
                midi.clear();

                for (int i = 0; i < testBlockSize; ++i)
                    total += std::abs (static_cast<double> (buffer.getSample (0, i)));
            }

            return total;
        }

        static void apply (TestProcessor& processor, const nog::presets::Preset& preset,
                           bool loadSamples = true)
        {
            for (const auto& [id, value] : preset.values)
                if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (
                        processor.apvts.getParameter (id)))
                    parameter->setValueNotifyingHost (
                        juce::jlimit (0.0f, 1.0f, parameter->convertTo0to1 (value)));

            // Samples are not parameters, so setting the values alone leaves a
            // sample patch pointing at an empty slot. Without this the tests
            // below would pass on silence for every instrument preset.
            if (! loadSamples)
                return;

            for (int i = 0; i < static_cast<int> (preset.builtInSamples.size()); ++i)
                if (const auto builtIn = preset.builtInSamples[static_cast<size_t> (i)]; builtIn >= 0)
                    processor.engine.getSampleLibrary().loadBuiltIn (i, builtIn);
        }
    };

    // -----------------------------------------------------------------------
    class SampleOscillatorTests final : public juce::UnitTest
    {
    public:
        SampleOscillatorTests() : UnitTest ("Sample oscillator", "nog") {}

        void runTest() override
        {
            // A short recognisable file written to disk, so the whole path is
            // exercised: decoding, normalising, pitching and playback.
            const auto file = juce::File::createTempFile (".wav");
            const auto writeSucceeded = writeTestTone (file, 220.0, 0.5, 44100.0);

            beginTest ("a test file can be written");
            expect (writeSucceeded, "could not create a temporary wav to test with");

            if (! writeSucceeded)
                return;

            beginTest ("a file loads, normalises and reports its length");
            {
                nog::dsp::SampleLibrary library;

                expect (library.getSlot (0) == nullptr, "slots start empty");
                expect (library.loadIntoSlot (0, file), "the file should load");

                const auto* sample = library.getSlot (0);
                expect (sample != nullptr);

                if (sample == nullptr)
                    return;

                expectWithinAbsoluteError (static_cast<double> (sample->getLength()), 22050.0, 64.0);
                expectWithinAbsoluteError (sample->getSourceSampleRate(), 44100.0, 1.0);

                auto peak = 0.0f;

                for (int i = 0; i < 512; ++i)
                    peak = juce::jmax (peak, std::abs (sample->read (0, i / 512.0)));

                // Loading normalises, so a quiet file and a loud one arrive at
                // the oscillator at the same level.
                expect (peak > 0.7f, "sample should be normalised, peak was " + juce::String (peak));
            }

            beginTest ("unison spread fans the stack across the table");
            {
                // Detune alone gives every unison voice the same waveform at a
                // different pitch. Spread gives them different waveforms too,
                // which has to be audible or the control does nothing.
                const auto renderWith = [] (float spread)
                {
                    nog::dsp::Oscillator oscillator;
                    oscillator.prepare (testSampleRate);
                    oscillator.setTable (&nog::dsp::WavetableBank::factory().getTable (1));

                    nog::dsp::Oscillator::Settings settings;
                    settings.unisonVoices = 7;
                    settings.detune       = 0.2f;
                    settings.morph        = 0.5f;
                    settings.tableSpread  = spread;
                    settings.phaseRandom  = 0.0f;
                    settings.level        = 1.0f;
                    oscillator.setSettings (settings);

                    oscillator.setFrequency (220.0f);
                    oscillator.noteOn();

                    std::vector<float> out (4096);

                    for (auto& value : out)
                    {
                        auto left = 0.0f, right = 0.0f;
                        oscillator.addNextSample (left, right);
                        value = left;
                    }

                    return out;
                };

                const auto flat   = renderWith (0.0f);
                const auto fanned = renderWith (1.0f);

                auto difference = 0.0;

                for (size_t i = 0; i < flat.size(); ++i)
                {
                    expect (std::isfinite (fanned[i]));
                    difference += std::abs (static_cast<double> (flat[i] - fanned[i]));
                }

                expect (difference > 1.0,
                        "spread should change the sound, total difference was "
                            + juce::String (difference, 4));
            }

            beginTest ("a one-shot sample plays at full level from its start");
            {
                // One-shot is what every instrument patch uses, and it takes a
                // different path through the oscillator than the looping mode
                // the test below covers. A one-shot that comes out quiet, or
                // that stops early, is silently wrong rather than obviously so.
                nog::dsp::SampleLibrary library;
                expect (library.loadBuiltIn (0, 5), "the grand piano should load");

                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);
                oscillator.setSample (library.getSlot (0));

                nog::dsp::Oscillator::Settings settings;
                settings.mode     = static_cast<int> (nog::dsp::Oscillator::Mode::Sample);
                settings.loop     = static_cast<int> (nog::dsp::Oscillator::Loop::OneShot);
                settings.rootNote = nog::dsp::SampleBank::getRootNote (5);
                settings.level    = 1.0f;
                settings.morph    = 0.0f;
                oscillator.setSettings (settings);

                oscillator.setFrequency (440.0f * std::exp2 ((settings.rootNote - 69.0f) / 12.0f));
                oscillator.noteOn();

                auto peak = 0.0f;

                for (int i = 0; i < 8192; ++i)
                {
                    auto left = 0.0f, right = 0.0f;
                    oscillator.addNextSample (left, right);
                    peak = juce::jmax (peak, std::abs (left));
                }

                expect (peak > 0.3f,
                        "a one-shot should play near full level, peak was " + juce::String (peak, 6));
            }

            beginTest ("a rubbish file is refused rather than half-loaded");
            {
                nog::dsp::SampleLibrary library;
                const auto notAudio = juce::File::createTempFile (".wav");
                notAudio.replaceWithText ("this is not a wav file");

                expect (! library.loadIntoSlot (0, notAudio));
                expect (library.getSlot (0) == nullptr);

                notAudio.deleteFile();
            }

            beginTest ("an oscillator in sample mode produces audio at the right pitch");
            {
                nog::dsp::SampleLibrary library;
                expect (library.loadIntoSlot (0, file));

                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);
                oscillator.setSample (library.getSlot (0));

                nog::dsp::Oscillator::Settings settings;
                settings.mode     = static_cast<int> (nog::dsp::Oscillator::Mode::Sample);
                settings.loop     = static_cast<int> (nog::dsp::Oscillator::Loop::Forward);
                settings.rootNote = 60;
                settings.level    = 1.0f;
                oscillator.setSettings (settings);

                expect (oscillator.isPlayingSample());

                // Played at its root note, the file should come out at its
                // original speed regardless of the host's sample rate.
                oscillator.setFrequency (440.0f * std::exp2 ((60.0f - 69.0f) / 12.0f));
                oscillator.noteOn();

                auto peak = 0.0f;

                for (int i = 0; i < 8192; ++i)
                {
                    auto left = 0.0f, right = 0.0f;
                    oscillator.addNextSample (left, right);

                    expect (std::isfinite (left) && std::isfinite (right));
                    peak = juce::jmax (peak, std::abs (left));
                }

                expect (peak > 0.1f, "sample playback was silent");
            }

            beginTest ("a one-shot stops at the end and a loop does not");
            {
                nog::dsp::SampleLibrary library;
                expect (library.loadIntoSlot (0, file));

                const auto measureTail = [&library] (nog::dsp::Oscillator::Loop loop)
                {
                    nog::dsp::Oscillator oscillator;
                    oscillator.prepare (testSampleRate);
                    oscillator.setSample (library.getSlot (0));

                    nog::dsp::Oscillator::Settings settings;
                    settings.mode  = static_cast<int> (nog::dsp::Oscillator::Mode::Sample);
                    settings.loop  = static_cast<int> (loop);
                    settings.level = 1.0f;
                    oscillator.setSettings (settings);
                    oscillator.setFrequency (440.0f * std::exp2 ((60.0f - 69.0f) / 12.0f));
                    oscillator.noteOn();

                    // Half a second of source at 44.1k, played at 48k, runs out
                    // well inside this.
                    for (int i = 0; i < 40000; ++i)
                    {
                        auto l = 0.0f, r = 0.0f;
                        oscillator.addNextSample (l, r);
                    }

                    auto peak = 0.0f;

                    for (int i = 0; i < 4096; ++i)
                    {
                        auto l = 0.0f, r = 0.0f;
                        oscillator.addNextSample (l, r);
                        peak = juce::jmax (peak, std::abs (l));
                    }

                    return peak;
                };

                expectWithinAbsoluteError (measureTail (nog::dsp::Oscillator::Loop::OneShot), 0.0f, 1.0e-6f);
                expect (measureTail (nog::dsp::Oscillator::Loop::Forward) > 0.1f,
                        "a looping sample should still be sounding");
            }

            beginTest ("every built-in character sample generates usable audio");
            {
                // These are synthesised rather than recorded, so a bad formula
                // would otherwise show up as a silent or exploding oscillator
                // with nothing to inspect.
                const auto& bank = nog::dsp::SampleBank::factory();
                const auto names = nog::dsp::SampleBank::getNames();

                expect (names.size() == nog::dsp::SampleBank::getCount());
                expect (names.size() >= 8, "expected a useful set of built-ins");

                for (int i = 0; i < nog::dsp::SampleBank::getCount(); ++i)
                {
                    const auto sample = bank.get (i);

                    expect (sample != nullptr, names[i] + " failed to generate");

                    if (sample == nullptr)
                        continue;

                    expect (sample->getLength() > 1000, names[i] + " is too short to be useful");

                    auto peak = 0.0f;
                    auto energy = 0.0;

                    for (int n = 0; n < 4096; ++n)
                    {
                        const auto value = sample->read (0, n / 4096.0);

                        expect (std::isfinite (value), names[i] + " contains non-finite samples");

                        peak = juce::jmax (peak, std::abs (value));
                        energy += static_cast<double> (value) * value;
                    }

                    expect (peak > 0.05f, names[i] + " is effectively silent");

                    // Normalisation puts the stored peak at 1, but reading is
                    // cubic and a cubic overshoots at a discontinuity - the
                    // chiptune samples are hard-edged squares, so an
                    // interpolated read genuinely exceeds the stored maximum.
                    // The bound is here to catch a runaway, not the overshoot.
                    expect (peak <= 1.5f,
                            names[i] + " is far above unity (peak " + juce::String (peak) + ")");
                    expect (energy > 1.0, names[i] + " carries almost no energy");
                }
            }

            beginTest ("built-in instruments are in tune with their root note");
            {
                // A modelled string or bar is only useful if it plays the pitch
                // it says it does: the oscillator transposes from the declared
                // root, so a sample that is a semitone out is a sample that is
                // out of tune in every patch that uses it.
                const auto& bank  = nog::dsp::SampleBank::factory();
                const auto  names = nog::dsp::SampleBank::getNames();

                for (int i = 0; i < nog::dsp::SampleBank::getCount(); ++i)
                {
                    if (! nog::dsp::SampleBank::isPitched (i))
                        continue;

                    const auto root   = nog::dsp::SampleBank::getRootNote (i);
                    const auto sample = bank.get (i);

                    if (sample == nullptr)
                        continue;

                    const auto expected = 440.0 * std::pow (2.0, (root - 69) / 12.0);
                    const auto measured = estimateFundamental (*sample);

                    // Two percent is about a third of a semitone: tight enough
                    // to catch an octave or semitone error, loose enough for
                    // the delay line to be quantised to a whole sample.
                    expect (std::abs (measured - expected) / expected < 0.02,
                            names[i] + " should sound near " + juce::String (expected, 1)
                                + " Hz but measured " + juce::String (measured, 1) + " Hz");
                }
            }

            file.deleteFile();
        }

    private:
        /** Frequency of the lowest partial that is clearly present.

            Not autocorrelation: half of these instruments are struck bars,
            whose modes sit at ratios like 2.76 and 5.40 rather than at whole
            multiples. Such a tone has no period at all, so a periodicity
            estimate lands on whatever pseudo-period the beating produces. The
            lowest prominent partial is well defined for both kinds of source,
            and it is what "the note it plays" means for a bar.
        */
        static double estimateFundamental (const nog::dsp::Sample& sample)
        {
            constexpr int order = 15;
            constexpr int size  = 1 << order;

            const auto length = sample.getLength();
            const auto rate   = sample.getSourceSampleRate();

            // Skip the attack: the pick or hammer noise is broadband and would
            // put energy in every bin.
            const auto start = static_cast<int> (rate * 0.05);


            juce::dsp::FFT fft (order);
            std::vector<float> data (static_cast<size_t> (size) * 2, 0.0f);

            for (int n = 0; n < size; ++n)
            {
                const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                            * static_cast<float> (n)
                                                            / static_cast<float> (size - 1));

                // Wraps round for the short ones. Every sample shorter than the
                // window is a seamless loop, so reading past the end is exactly
                // what playback does rather than a distortion of it.
                const auto position = (start + n) % length;

                data[static_cast<size_t> (n)] =
                    sample.read (0, position / static_cast<double> (length)) * window;
            }

            fft.performFrequencyOnlyForwardTransform (data.data());

            const auto binToHz = rate / size;
            const auto lowest  = juce::jmax (1, static_cast<int> (35.0 / binToHz));
            const auto highest = juce::jmin (size / 2 - 2, static_cast<int> (2500.0 / binToHz));

            auto strongest = 0.0f;

            for (int bin = lowest; bin <= highest; ++bin)
                strongest = juce::jmax (strongest, data[static_cast<size_t> (bin)]);

            for (int bin = lowest + 1; bin < highest; ++bin)
            {
                const auto here  = data[static_cast<size_t> (bin)];
                const auto below = data[static_cast<size_t> (bin - 1)];
                const auto above = data[static_cast<size_t> (bin + 1)];

                if (here < strongest * 0.1f || here < below || here < above)
                    continue;

                // Parabolic interpolation across the peak, so the estimate is
                // not limited to the bin spacing.
                const auto denominator = below - 2.0 * here + above;
                const auto offset = denominator != 0.0 ? 0.5 * (below - above) / denominator : 0.0;

                return (bin + offset) * binToHz;
            }

            return 0.0;
        }

        /** Writes a fixed sine to @p file so the tests have something real to
            decode rather than a synthetic buffer. */
        static bool writeTestTone (const juce::File& file, double frequency,
                                   double seconds, double sampleRate)
        {
            const auto length = static_cast<int> (sampleRate * seconds);

            juce::AudioBuffer<float> buffer (1, length);

            for (int i = 0; i < length; ++i)
                buffer.setSample (0, i, 0.4f * std::sin (juce::MathConstants<float>::twoPi
                                                         * static_cast<float> (frequency * i / sampleRate)));

            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());

            if (stream == nullptr)
                return false;

            const auto options = juce::AudioFormatWriterOptions()
                                     .withSampleRate (sampleRate)
                                     .withNumChannels (1)
                                     .withBitsPerSample (16);

            // Takes the stream by reference and claims it on success.
            const auto writer = format.createWriterFor (stream, options);

            if (writer == nullptr)
                return false;

            return writer->writeFromAudioSampleBuffer (buffer, 0, length);
        }
    };

    // -----------------------------------------------------------------------
    class ArpeggiatorTests final : public juce::UnitTest
    {
    public:
        ArpeggiatorTests() : UnitTest ("Arpeggiator", "nog") {}

        void runTest() override
        {
            beginTest ("a held chord produces a steady stream of note-ons");
            {
                // Counted from the arpeggiator directly rather than inferred
                // from the output level: with sustain and release the notes
                // overlap, so the audio never drops to silence between steps
                // even though it is retriggering correctly.
                nog::Arpeggiator arp;
                arp.prepare (testSampleRate);

                nog::Arpeggiator::Settings settings;
                settings.enabled  = true;
                settings.division = 7;   // sixteenths
                settings.gate     = 0.5f;

                for (const auto note : { 60, 64, 67 })
                    arp.noteOn (note, 0.9f);

                std::vector<nog::Arpeggiator::Event> events;

                // Two seconds at 120 bpm in sixteenths is sixteen steps.
                for (int block = 0; block < 190; ++block)
                    arp.process (testBlockSize, 120.0, settings, events);

                auto noteOns = 0;

                for (const auto& event : events)
                    if (event.isNoteOn)
                        ++noteOns;

                logMessage ("note-ons in two seconds: " + juce::String (noteOns));
                expect (noteOns >= 12,
                        "expected roughly sixteen steps, got " + juce::String (noteOns));

                // Up mode over one octave should cycle through the three held
                // notes in ascending order.
                juce::Array<int> order;

                for (const auto& event : events)
                    if (event.isNoteOn && order.size() < 6)
                        order.add (event.note);

                expect (order.size() >= 6);
                expect (order[0] == 60 && order[1] == 64 && order[2] == 67,
                        "up mode should ascend through the held notes");
                expect (order[3] == 60, "the pattern should wrap round");
            }

            beginTest ("every note-on is matched by a note-off");
            {
                // A leaked note-on would leave a voice stuck on forever.
                nog::Arpeggiator arp;
                arp.prepare (testSampleRate);

                nog::Arpeggiator::Settings settings;
                settings.enabled = true;
                settings.gate    = 0.4f;

                arp.noteOn (60, 0.9f);
                arp.noteOn (67, 0.9f);

                std::vector<nog::Arpeggiator::Event> events;

                for (int block = 0; block < 190; ++block)
                    arp.process (testBlockSize, 120.0, settings, events);

                // Let go, then run on so the final note is released.
                arp.noteOff (60);
                arp.noteOff (67);

                for (int block = 0; block < 20; ++block)
                    arp.process (testBlockSize, 120.0, settings, events);

                std::map<int, int> balance;

                for (const auto& event : events)
                    balance[event.note] += event.isNoteOn ? 1 : -1;

                for (const auto& [note, count] : balance)
                    expect (count == 0,
                            "note " + juce::String (note) + " is unbalanced by " + juce::String (count));
            }

            beginTest ("the swing setting makes alternate steps uneven");
            {
                const auto measureFirstGap = [] (float swing)
                {
                    nog::Arpeggiator arp;
                    arp.prepare (testSampleRate);

                    nog::Arpeggiator::Settings settings;
                    settings.enabled = true;
                    settings.swing   = swing;

                    arp.noteOn (60, 0.9f);

                    std::vector<nog::Arpeggiator::Event> events;

                    for (int block = 0; block < 40; ++block)
                        arp.process (testBlockSize, 120.0, settings, events);

                    juce::Array<int> onsets;
                    auto absolute = 0;
                    auto blockIndex = 0;

                    juce::ignoreUnused (absolute, blockIndex);

                    auto count = 0;

                    for (const auto& event : events)
                        if (event.isNoteOn)
                            ++count;

                    return count;
                };

                // Swing redistributes time between pairs of steps rather than
                // changing the overall rate, so the count over a fixed window
                // should stay in the same region.
                const auto straight = measureFirstGap (0.0f);
                const auto swung    = measureFirstGap (0.6f);

                expect (straight > 0 && swung > 0);
                expect (std::abs (straight - swung) <= 2,
                        "swing should not change the average tempo");
            }

            beginTest ("releasing the keys stops it");
            {
                TestProcessor processor;
                processor.parameters.arpEnable->setValueNotifyingHost (1.0f);
                processor.prepareToPlay (testSampleRate, testBlockSize);

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::MidiBuffer midi;
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);

                for (int block = 0; block < 40; ++block)
                {
                    processor.processBlock (buffer, midi);
                    midi.clear();
                }

                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);

                for (int block = 0; block < 400; ++block)
                {
                    processor.processBlock (buffer, midi);
                    midi.clear();
                }

                expect (processor.engine.getActiveVoiceCount() == 0,
                        "letting go of the keys should stop the pattern");
            }

            beginTest ("every mode runs and stays healthy");
            {
                for (int mode = 0; mode < 7; ++mode)
                {
                    TestProcessor processor;
                    processor.parameters.arpEnable->setValueNotifyingHost (1.0f);
                    processor.parameters.arpMode->setValueNotifyingHost (
                        processor.parameters.arpMode->convertTo0to1 (static_cast<float> (mode)));
                    processor.parameters.arpOctaves->setValueNotifyingHost (
                        processor.parameters.arpOctaves->convertTo0to1 (3));
                    processor.prepareToPlay (testSampleRate, testBlockSize);

                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    juce::MidiBuffer midi;

                    for (const auto note : { 55, 60, 64 })
                        midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);

                    auto peak = 0.0f;

                    for (int block = 0; block < 120; ++block)
                    {
                        processor.processBlock (buffer, midi);
                        midi.clear();

                        expect (isBufferHealthy (buffer), "arp mode " + juce::String (mode) + " misbehaved");
                        peak = juce::jmax (peak, buffer.getMagnitude (0, testBlockSize));
                    }

                    expect (peak > 1.0e-3f, "arp mode " + juce::String (mode) + " was silent");
                }
            }

            beginTest ("a single held note still repeats");
            {
                // A one-note pattern is the degenerate case, and the one most
                // likely to stall or divide by zero.
                nog::Arpeggiator arp;
                arp.prepare (testSampleRate);

                nog::Arpeggiator::Settings settings;
                settings.enabled = true;

                arp.noteOn (60, 0.9f);

                std::vector<nog::Arpeggiator::Event> events;

                for (int block = 0; block < 190; ++block)
                    arp.process (testBlockSize, 120.0, settings, events);

                auto noteOns = 0;

                for (const auto& event : events)
                    if (event.isNoteOn)
                        ++noteOns;

                expect (noteOns >= 12, "a single held note should still retrigger");
            }
        }
    };

    ParameterTests          parameterTests;
    ArpeggiatorTests        arpeggiatorTests;
    SampleOscillatorTests   sampleOscillatorTests;
    FactoryPresetTests  factoryPresetTests;
    WavetableTests  wavetableTests;
    ModMatrixTests  modMatrixTests;
    EnvelopeTests   envelopeTests;
    OscillatorTests oscillatorTests;
    SynthEngineTests synthEngineTests;
}

int main (int, char**)
{
    // The message manager has to exist before anything JUCE-owned is created,
    // but no window is ever opened, so this stays headless.
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    // Only this project's tests: JUCE registers its own into the same global
    // list, and running those here would test the framework, not the plugin.
    runner.runTestsInCategory ("nog");

    auto failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult (i);

        if (result != nullptr)
            failures += result->failures;
    }

    if (failures > 0)
    {
        std::cerr << failures << " test failure(s)" << std::endl;
        return 1;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
