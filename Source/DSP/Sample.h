#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace nog::dsp
{
    /**
        An audio file loaded into memory, ready to be played back at any pitch.

        Reference counted because a voice may still be reading one on the audio
        thread when the user loads a replacement. The library keeps every sample
        it has loaded alive for the lifetime of the session, so the audio thread
        never has to touch a reference count or risk running a destructor.

        Playback position is expressed as a normalised 0..1 fraction of the
        file, which lets the oscillator drive a sample with exactly the same
        phase machinery it uses for a wavetable.
    */
    class Sample : public juce::ReferenceCountedObject
    {
    public:
        using Ptr = juce::ReferenceCountedObjectPtr<Sample>;

        /** Reads @p file into memory. Returns nullptr if it cannot be decoded.

            Must be called from the message thread: it allocates and blocks on
            disk.
        */
        static Ptr load (const juce::File& file, juce::AudioFormatManager& formats);

        /** Wraps audio that was generated rather than read from disk.
            Normalises in the same way loading does. */
        static Ptr fromBuffer (juce::String sampleName, juce::AudioBuffer<float> audio,
                               double rate);

        const juce::String& getName() const noexcept { return name; }
        int getNumChannels() const noexcept { return buffer.getNumChannels(); }
        int getLength() const noexcept { return buffer.getNumSamples(); }
        double getSourceSampleRate() const noexcept { return sourceSampleRate; }

        /** Playback rate that reproduces the file at its original speed when
            the host runs at @p targetSampleRate. */
        double getBaseRatio (double targetSampleRate) const noexcept
        {
            return targetSampleRate > 0.0 ? sourceSampleRate / targetSampleRate : 1.0;
        }

        /** One interpolated sample. @p position is 0..1 across the file.

            Uses the same Catmull-Rom interpolation as the wavetables. Playing a
            sample far above its original pitch will still alias - unlike a
            wavetable there is no band-limited pyramid to fall back on - but the
            cubic keeps that well below what linear interpolation would give.
        */
        float read (int channel, double position) const noexcept;

    private:
        // Declaring the copy constructor deleted (via the macro below) suppresses
        // the implicit default one, so it has to be asked for explicitly.
        Sample() = default;

        juce::String name;
        juce::AudioBuffer<float> buffer;
        double sourceSampleRate = 44100.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sample)
    };
}
