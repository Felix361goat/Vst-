#include "DSP/Sample.h"

namespace nog::dsp
{
    namespace
    {
        /** Hard ceiling on how much audio a single slot will hold. A user
            dropping in a ten minute file should get a truncated sample rather
            than a gigabyte of resident memory per plugin instance. */
        constexpr double maximumSeconds = 30.0;
    }

    Sample::Ptr Sample::load (const juce::File& file, juce::AudioFormatManager& formats)
    {
        if (! file.existsAsFile())
            return {};

        const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr || reader->numChannels == 0 || reader->lengthInSamples <= 0)
            return {};

        const auto maximumLength = static_cast<juce::int64> (reader->sampleRate * maximumSeconds);
        const auto length = static_cast<int> (juce::jmin (reader->lengthInSamples, maximumLength));

        // Everything past stereo is discarded; the oscillator is a stereo path.
        const auto channels = static_cast<int> (juce::jmin (reader->numChannels, 2u));

        Ptr sample (new Sample());
        sample->name = file.getFileNameWithoutExtension();
        sample->sourceSampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
        sample->buffer.setSize (channels, length);

        if (! reader->read (&sample->buffer, 0, length, 0, true, channels > 1))
            return {};

        // Normalise so that swapping samples does not swing the patch level
        // around; the oscillator's own level control is what should set it.
        const auto peak = sample->buffer.getMagnitude (0, length);

        if (peak > 1.0e-6f)
            sample->buffer.applyGain (1.0f / peak);
        else
            return {};   // silent file: nothing to play

        return sample;
    }

    Sample::Ptr Sample::fromBuffer (juce::String sampleName, juce::AudioBuffer<float> audio, double rate)
    {
        if (audio.getNumSamples() <= 0 || audio.getNumChannels() <= 0)
            return {};

        const auto peak = audio.getMagnitude (0, audio.getNumSamples());

        if (peak <= 1.0e-6f)
            return {};

        Ptr sample (new Sample());
        sample->name = std::move (sampleName);
        sample->sourceSampleRate = rate > 0.0 ? rate : 44100.0;
        sample->buffer = std::move (audio);
        sample->buffer.applyGain (1.0f / peak);

        return sample;
    }

    float Sample::read (int channel, double position) const noexcept
    {
        const auto length = buffer.getNumSamples();

        if (length <= 0)
            return 0.0f;

        const auto sourceChannel = juce::jlimit (0, buffer.getNumChannels() - 1, channel);
        const auto* data = buffer.getReadPointer (sourceChannel);

        const auto exact = juce::jlimit (0.0, 1.0, position) * static_cast<double> (length - 1);
        const auto index = static_cast<int> (exact);
        const auto fraction = static_cast<float> (exact - static_cast<double> (index));

        // Clamped rather than wrapped at the edges: a sample is not periodic,
        // so wrapping would splice the end onto the start.
        const auto at = [data, length] (int i) noexcept
        {
            return data[juce::jlimit (0, length - 1, i)];
        };

        const auto x0 = at (index - 1);
        const auto x1 = at (index);
        const auto x2 = at (index + 1);
        const auto x3 = at (index + 2);

        const auto a = -0.5f * x0 + 1.5f * x1 - 1.5f * x2 + 0.5f * x3;
        const auto b =         x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
        const auto c = -0.5f * x0                + 0.5f * x2;

        return ((a * fraction + b) * fraction + c) * fraction + x1;
    }
}
