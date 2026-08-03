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

        Sample::Ptr get (int index) const;

    private:
        SampleBank();

        std::vector<Sample::Ptr> samples;
    };
}
