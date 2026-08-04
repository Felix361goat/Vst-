#pragma once

#include <array>
#include <vector>

#include "Modulation/ModDefs.h"
#include "Params/ParameterStore.h"

namespace nog
{
    /**
        One voice's modulation state for the current sub-block.

        Sources are sampled into `sources`, the matrix is applied, and the result
        lands in `offsets` as a normalised nudge per destination. Keeping both
        arrays flat and indexed by enum means applying the matrix is a walk over
        a short list of integers with no branching on names or strings.
    */
    struct ModulationFrame
    {
        std::array<float, mod::numSources> sources {};
        std::array<float, mod::numDests>   offsets {};

        void clearOffsets() noexcept { offsets.fill (0.0f); }

        void setSource (mod::Source s, float value) noexcept
        {
            sources[static_cast<size_t> (s)] = value;
        }

        float getSource (mod::Source s) const noexcept
        {
            return sources[static_cast<size_t> (s)];
        }

        float getOffset (mod::Dest d) const noexcept
        {
            return offsets[static_cast<size_t> (d)];
        }
    };

    /**
        The routing table shared by every voice.

        Slot parameters live in the APVTS so they automate and save like any
        other parameter, but reading sixteen slots' worth of choice and float
        parameters per voice per sub-block would be wasteful. Instead refresh()
        collapses them once per block into a compact list of active routings, and
        voices apply that list.
    */
    class ModMatrix
    {
    public:
        struct Routing
        {
            mod::Source source  = mod::Source::None;
            mod::Dest   dest    = mod::Dest::None;
            float       amount  = 0.0f;
            bool        bipolar = false;

            /** Scales the slot by a second source. None means full depth. */
            mod::Source via     = mod::Source::None;
        };

        ModMatrix();

        /** Rebuilds the active routing list. Call once per processBlock. */
        void refresh (const ParameterStore& parameters);

        /** Samples the sources that are the same for every voice - macros and
            MIDI controllers - into @p frame. */
        void applyGlobalSources (ModulationFrame& frame) const noexcept;

        /** Applies every active routing, accumulating into frame.offsets.

            Sources must already be populated. Offsets are *not* cleared here so
            that a caller can layer several passes if it needs to.
        */
        void apply (ModulationFrame& frame) const noexcept;

        /** Total modulation reaching a destination across all slots, ignoring
            the source values. Used by the editor to draw modulation ranges. */
        float getModulationDepth (mod::Dest dest) const noexcept;

        void setMacro (int index, float value) noexcept;

        /** The global step patterns' current levels. Held here so that voices
            see them alongside the macros: Motion exists to drive the effects
            rack, but there is no reason a voice should not use it too. */
        void setMotion (int index, float value) noexcept;
        void setModWheel (float value) noexcept     { modWheel = value; }
        void setPitchBend (float value) noexcept    { pitchBend = value; }
        void setAftertouch (float value) noexcept   { aftertouch = value; }

        float getPitchBend() const noexcept  { return pitchBend; }

        const std::vector<Routing>& getRoutings() const noexcept { return routings; }

    private:
        // Reserved at construction so refresh() never allocates on the audio
        // thread; it can only ever hold numMatrixSlots entries.
        std::vector<Routing> routings;

        std::array<float, ids::numMacros>  macros {};
        std::array<float, ids::numMotions> motionValues {};
        float modWheel   = 0.0f;
        float pitchBend  = 0.0f;
        float aftertouch = 0.0f;
    };
}
