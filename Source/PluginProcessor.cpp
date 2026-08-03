#include "PluginProcessor.h"

#include "Params/ParameterLayout.h"
#include "PluginEditor.h"

namespace nog
{
    namespace
    {
        constexpr const char* stateTypeName       = "NogSuiteState";
        constexpr const char* stateVersionProperty = "stateVersion";
        constexpr const char* editorWidthProperty  = "editorWidth";
        constexpr const char* editorHeightProperty = "editorHeight";

        /** Longest tail the effects rack can produce, added to the envelope
            release when reporting the tail length to the host. */
        constexpr double effectsTailSeconds = 4.0;
    }

    juce::AudioProcessor::BusesProperties NogSuiteProcessor::getBusesLayout()
    {
        // An instrument with no input bus at all. Declaring a disabled input bus
        // instead is a common cause of hosts refusing to load a synth, so there
        // simply is not one.
        return BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true);
    }

    NogSuiteProcessor::NogSuiteProcessor()
        : AudioProcessor (getBusesLayout()),
          apvts (*this, nullptr, juce::Identifier (stateTypeName), params::createLayout()),
          engine (parameters),
          presets (apvts)
    {
        parameters.attach (apvts);
    }

    NogSuiteProcessor::~NogSuiteProcessor() = default;

    bool NogSuiteProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        // Instruments produce audio and consume none.
        if (layouts.getMainInputChannels() != 0)
            return false;

        const auto output = layouts.getMainOutputChannelSet();

        return output == juce::AudioChannelSet::stereo()
            || output == juce::AudioChannelSet::mono();
    }

    void NogSuiteProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
    {
        engine.prepare (sampleRate, maximumExpectedSamplesPerBlock, getTotalNumOutputChannels());

        // Oversampling filters delay the signal; the host has to know so it can
        // line the plugin's output up with everything else.
        setLatencySamples (engine.getLatencySamples());
    }

    void NogSuiteProcessor::releaseResources()
    {
        engine.reset();
    }

    double NogSuiteProcessor::getTailLengthSeconds() const
    {
        auto longestRelease = 0.0f;

        for (int i = 0; i < ids::numEnvelopes; ++i)
            if (auto* release = parameters.env[static_cast<size_t> (i)].release)
                longestRelease = juce::jmax (longestRelease, release->get());

        return static_cast<double> (longestRelease) * 0.001 + effectsTailSeconds;
    }

    void NogSuiteProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        juce::ScopedNoDenormals noDenormals;

        // Voices accumulate into the buffer, so it has to start silent.
        buffer.clear();

        auto bpm = 120.0;

        if (auto* transport = getPlayHead())
            if (const auto position = transport->getPosition())
                if (const auto hostBpm = position->getBpm())
                    bpm = *hostBpm;

        engine.process (buffer, midiMessages, bpm);

        // The oversampling setting can change between blocks, and with it the
        // reported latency.
        if (const auto latency = engine.getLatencySamples(); latency != getLatencySamples())
            setLatencySamples (latency);

        // The synth generates no MIDI; leaving incoming events in the buffer
        // would make some hosts echo them back out.
        midiMessages.clear();

        auto peak = 0.0f;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = juce::jmax (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));

        outputLevel.store (peak, std::memory_order_relaxed);
        activeVoices.store (engine.getActiveVoiceCount(), std::memory_order_relaxed);
    }

    juce::AudioProcessorEditor* NogSuiteProcessor::createEditor()
    {
        return new NogSuiteEditor (*this);
    }

    juce::Point<int> NogSuiteProcessor::getSavedEditorSize() const
    {
        const auto width  = static_cast<int> (apvts.state.getProperty (editorWidthProperty, 0));
        const auto height = static_cast<int> (apvts.state.getProperty (editorHeightProperty, 0));

        return { width, height };
    }

    void NogSuiteProcessor::setSavedEditorSize (juce::Point<int> size)
    {
        apvts.state.setProperty (editorWidthProperty,  size.x, nullptr);
        apvts.state.setProperty (editorHeightProperty, size.y, nullptr);
    }

    void NogSuiteProcessor::getStateInformation (juce::MemoryBlock& destData)
    {
        auto state = apvts.copyState();
        state.setProperty (stateVersionProperty, currentStateVersion, nullptr);

        if (const auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }

    void NogSuiteProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        const auto xml = getXmlFromBinary (data, sizeInBytes);

        if (xml == nullptr)
            return;

        // Refuse anything that is not ours rather than letting APVTS silently
        // apply a foreign tree.
        if (! xml->hasTagName (apvts.state.getType()))
            return;

        auto tree = juce::ValueTree::fromXml (*xml);

        if (! tree.isValid())
            return;

        const auto version = static_cast<int> (tree.getProperty (stateVersionProperty, 1));

        // Migration point. Nothing to do for version 1; when the format
        // changes, convert older trees here before handing them to the APVTS.
        // Parameters missing from an older state keep their default values,
        // which is what makes adding parameters a backwards-compatible change.
        if (version > currentStateVersion)
        {
            // Saved by a newer build. Loading it anyway is still the best
            // option: known parameters are restored and unknown ones ignored.
            jassertfalse;
        }

        apvts.replaceState (tree);
    }
}

// The entry point the host calls to construct the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new nog::NogSuiteProcessor();
}
