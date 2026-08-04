#pragma once

#include <vector>

#include "DSP/Sample.h"

namespace nog::dsp
{
    /**
        Character samples that ship with the plugin.

        These are *generated*, not recorded. That is a deliberate choice: it
        keeps the download small, makes them identical on every machine and in
        every session, and sidesteps the licensing problem that comes with
        bundling recordings of anything.

        What they are for is the material a wavetable cannot provide. A
        wavetable is one cycle repeated forever, so it is periodic by
        construction - it can never be a transient, a burst of noise, or
        something whose spectrum evolves over its own length. Everything here is
        one of those: arcade blips, tape hiss, metal hits, crackle.

        Built once per process and shared, exactly like the wavetable bank.
    */
    class SampleBank
    {
    public:
        static const SampleBank& factory();

        /** Names, available without generating anything. */
        static juce::StringArray getNames();
        static int getCount();

        /** True for the ones meant to be looped rather than played as a hit. */
        static bool isLooping (int index);

        /** The MIDI note the sample was generated at. Loading a built-in sets
            the oscillator's root from this, so a Grand Piano lands in tune
            without the player having to work out what pitch it was made at. */
        static int getRootNote (int index);

        /** True for the modelled instruments, which have a definite pitch. The
            noise and blip samples do not, and their root note is a placeholder. */
        static bool isPitched (int index);

        Sample::Ptr get (int index) const;

        /** The texture behind a noise colour, or nullptr for the generated
            ones. Kept here rather than in the noise generator so the indices
            live beside the table that defines them. */
        static int noiseTextureIndex (int colour);

    private:
        SampleBank();

        std::vector<Sample::Ptr> samples;
    };
}
