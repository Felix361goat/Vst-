#pragma once

#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

namespace nog
{
    /**
        Turns held notes into a repeating pattern locked to the host tempo.

        The synth had categories called Arp and Sequence but no arpeggiator, so
        those patches played a single note and stopped. This is what makes them
        do what their names promise.

        It sits between the MIDI input and voice allocation: while it is on,
        note-ons go into a held list rather than straight to a voice, and the
        step clock decides what actually sounds and when.
    */
    class Arpeggiator
    {
    public:
        /** Order must match params::choices::arpModes(). */
        enum class Mode { Up = 0, Down, UpDown, DownUp, AsPlayed, Random, Chord };

        struct Settings
        {
            bool  enabled  = false;
            int   mode     = 0;
            int   division = 7;      // index into tempoDivisions(); 7 is a sixteenth
            int   octaves  = 1;      // 1..4
            float gate     = 0.5f;   // fraction of a step the note is held for
            float swing    = 0.0f;   // 0 straight, 1 heavily swung
        };

        /** What the engine should do at a given sample offset in the block. */
        struct Event
        {
            int   offset   = 0;
            int   note     = 60;
            float velocity = 1.0f;
            bool  isNoteOn = true;
        };

        void prepare (double newSampleRate);
        void reset();

        void noteOn (int note, float velocity);
        void noteOff (int note);

        /** True while any key is held. */
        bool hasHeldNotes() const noexcept { return ! held.empty(); }

        /** Advances the clock across @p numSamples and appends what it produced
            to @p events. Offsets are relative to the start of the segment. */
        void process (int numSamples, double bpm, const Settings& settings,
                      std::vector<Event>& events);

        /** Samples until the next step boundary, for callers that want to split
            their rendering there. Returns a large number when idle. */
        int samplesUntilNextStep (double bpm, const Settings& settings) const noexcept;

        /** True while the arp has notes it still has to release. */
        bool hasSoundingNotes() const noexcept { return ! sounding.empty(); }

    private:
        struct HeldNote
        {
            int   note;
            float velocity;
        };

        /** Rebuilds the pattern from the held notes and the current mode. */
        void rebuildPattern (const Settings& settings);

        double stepLengthInSamples (double bpm, const Settings& settings) const noexcept;

        double sampleRate = 44100.0;

        std::vector<HeldNote> held;       // in the order they were pressed
        std::vector<HeldNote> pattern;    // held notes expanded by mode and octaves

        double stepPhase   = 0.0;   // samples into the current step
        double currentStepLength = 0.0;   // its length, with swing already applied
        int    stepIndex   = 0;
        /** Every note the arp has started and not yet released. A vector
            rather than a single note because chord mode starts the whole
            pattern at once, and something has to release all of it. */
        std::vector<int> sounding;
        double gateRemaining = 0.0;
        bool   descending  = false; // for the up-down modes

        juce::Random random;
    };
}
