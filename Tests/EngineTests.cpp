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

#include "Engine/SynthEngine.h"
#include "DSP/Envelope.h"
#include "DSP/Oscillator.h"
#include "Params/ParameterLayout.h"
#include "Params/ParameterStore.h"

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
    class OscillatorTests final : public juce::UnitTest
    {
    public:
        OscillatorTests() : UnitTest ("Oscillator", "nog") {}

        void runTest() override
        {
            beginTest ("output stays finite across every wave, warp and unison count");
            {
                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);

                for (int wave = 0; wave < nog::dsp::Oscillator::numWaves; ++wave)
                {
                    for (int warp = 0; warp < 8; ++warp)
                    {
                        nog::dsp::Oscillator::Settings settings;
                        settings.wave         = wave;
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
                                    "wave " + juce::String (wave) + " warp " + juce::String (warp));
                            expect (std::abs (left) < 8.0f && std::abs (right) < 8.0f,
                                    "runaway level on wave " + juce::String (wave));
                        }
                    }
                }
            }

            beginTest ("frequencies at or above Nyquist are dropped rather than aliased");
            {
                nog::dsp::Oscillator oscillator;
                oscillator.prepare (testSampleRate);

                nog::dsp::Oscillator::Settings settings;
                settings.wave  = 2;
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

    ParameterTests  parameterTests;
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
