#pragma once

#include <array>
#include <juce_core/juce_core.h>

/**
    The modulation vocabulary: what can modulate, and what can be modulated.

    Adding a source or destination is a two-step edit - add the enum entry and
    add its display name to the matching table below. The static_asserts at the
    bottom of this file will fail the build if the two ever drift apart, which
    matters because the tables are what populate the matrix combo boxes and are
    serialised by index into user presets.

    Ordering is API: the choice-parameter index is the enum value, so new
    entries go at the end, immediately before Count.
*/
namespace nog::mod
{
    enum class Source : int
    {
        None = 0,
        Env1, Env2, Env3,
        Lfo1, Lfo2, Lfo3, Lfo4,
        Velocity,
        KeyTrack,
        ModWheel,
        PitchBend,
        Aftertouch,
        Random,
        Macro1, Macro2, Macro3, Macro4,
        Motion1, Motion2,
        Count
    };

    enum class Dest : int
    {
        None = 0,

        Osc1Level, Osc1Pan, Osc1Pitch, Osc1WtPos, Osc1Warp, Osc1Detune, Osc1Phase,
        Osc2Level, Osc2Pan, Osc2Pitch, Osc2WtPos, Osc2Warp, Osc2Detune, Osc2Phase,

        SubLevel, SubPan,
        NoiseLevel, NoisePan,

        FilterCutoff, FilterReso, FilterDrive, FilterMix,

        Lfo1Rate, Lfo2Rate, Lfo3Rate, Lfo4Rate,

        MasterGain,

        // The effects rack. Global rather than per-voice: the rack runs once on
        // the summed output, so only a source that belongs to the instrument
        // rather than to a note can drive it.
        Fx1Mix, Fx1A, Fx1B, Fx1C,
        Fx2Mix, Fx2A, Fx2B, Fx2C,
        Fx3Mix, Fx3A, Fx3B, Fx3C,
        Fx4Mix, Fx4A, Fx4B, Fx4C,
        Fx5Mix, Fx5A, Fx5B, Fx5C,
        Fx6Mix, Fx6A, Fx6B, Fx6C,

        Filter2Cutoff, Filter2Reso, Filter2Drive, Filter2Mix,
        Count
    };

    inline constexpr int numSources = static_cast<int> (Source::Count);
    inline constexpr int numDests   = static_cast<int> (Dest::Count);

    /** Display names, indexed by enum value. Order must match the enums. */
    inline const auto& sourceNames()
    {
        static constexpr std::array names {
            "None",
            "Env 1", "Env 2", "Env 3",
            "LFO 1", "LFO 2", "LFO 3", "LFO 4",
            "Velocity",
            "Key Track",
            "Mod Wheel",
            "Pitch Bend",
            "Aftertouch",
            "Random",
            "Macro 1", "Macro 2", "Macro 3", "Macro 4",
            "Motion 1", "Motion 2"
        };
        static_assert (names.size() == static_cast<size_t> (numSources),
                       "Source enum and sourceNames() table have drifted apart");
        return names;
    }

    inline const auto& destNames()
    {
        static constexpr std::array names {
            "None",

            "Osc 1 Level", "Osc 1 Pan", "Osc 1 Pitch", "Osc 1 WT Pos",
            "Osc 1 Warp",  "Osc 1 Detune", "Osc 1 Phase",

            "Osc 2 Level", "Osc 2 Pan", "Osc 2 Pitch", "Osc 2 WT Pos",
            "Osc 2 Warp",  "Osc 2 Detune", "Osc 2 Phase",

            "Sub Level", "Sub Pan",
            "Noise Level", "Noise Pan",

            "Filter Cutoff", "Filter Reso", "Filter Drive", "Filter Mix",

            "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate", "LFO 4 Rate",

            "Master Gain",

            "FX 1 Mix", "FX 1 A", "FX 1 B", "FX 1 C",
            "FX 2 Mix", "FX 2 A", "FX 2 B", "FX 2 C",
            "FX 3 Mix", "FX 3 A", "FX 3 B", "FX 3 C",
            "FX 4 Mix", "FX 4 A", "FX 4 B", "FX 4 C",
            "FX 5 Mix", "FX 5 A", "FX 5 B", "FX 5 C",
            "FX 6 Mix", "FX 6 A", "FX 6 B", "FX 6 C",

            "Filter 2 Cutoff", "Filter 2 Reso", "Filter 2 Drive", "Filter 2 Mix"
        };
        static_assert (names.size() == static_cast<size_t> (numDests),
                       "Dest enum and destNames() table have drifted apart");
        return names;
    }

    /** True for sources that are evaluated per-voice rather than globally.

        Envelopes, velocity, key tracking and the per-voice LFOs differ between
        simultaneously sounding notes, so they cannot be collapsed into a single
        global value the way a macro or the mod wheel can.
    */
    inline constexpr bool isPerVoice (Source s) noexcept
    {
        // Env1..Lfo4 are contiguous in the enum, so the range test covers the
        // seven of them; the rest are named individually.
        const auto value = static_cast<int> (s);

        const auto inEnvelopeAndLfoRange = value >= static_cast<int> (Source::Env1)
                                        && value <= static_cast<int> (Source::Lfo4);

        return inEnvelopeAndLfoRange
            || s == Source::Velocity
            || s == Source::KeyTrack
            || s == Source::Random;
    }

    /** True for destinations that are not backed by a single host parameter.

        Pitch is the motivating case: a note's frequency comes from octave,
        semitone and fine-tune parameters combined with the played key, so there
        is nothing sensible to nudge in normalised parameter space. Virtual
        destinations instead scale their modulation by a fixed full-scale amount
        (see virtualFullScale) and the voice adds the result directly.
    */
    inline constexpr bool isVirtual (Dest d) noexcept
    {
        return d == Dest::Osc1Pitch || d == Dest::Osc2Pitch;
    }

    /** True for destinations that live outside the voices.

        The effects rack processes the summed output once per block, so a voice
        has nothing to say about it - thirty-two of them would each have their
        own opinion. These are resolved from a single global modulation frame
        instead, and a routing into one of them from a per-voice source such as
        an envelope simply contributes nothing.
    */
    inline constexpr bool isGlobal (Dest d) noexcept
    {
        const auto value = static_cast<int> (d);

        return value >= static_cast<int> (Dest::Fx1Mix)
            && value <= static_cast<int> (Dest::Fx6C);
    }

    /** Modulation range of a virtual destination at an amount of 1.0.

        Semitones for the pitch destinations - +/- 4 octaves matches the range
        Serum gives a pitch envelope, which is enough for both subtle vibrato at
        low amounts and full-range pitch sweeps at high ones.
    */
    inline constexpr float virtualFullScale (Dest d) noexcept
    {
        return isVirtual (d) ? 48.0f : 0.0f;
    }

    inline juce::StringArray sourceChoices()
    {
        juce::StringArray a;
        for (auto* n : sourceNames())
            a.add (n);
        return a;
    }

    inline juce::StringArray destChoices()
    {
        juce::StringArray a;
        for (auto* n : destNames())
            a.add (n);
        return a;
    }
}
