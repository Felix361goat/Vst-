#pragma once

#include <vector>

#include "DSP/Wavetable.h"

namespace nog::dsp
{
    /**
        The built-in wavetables.

        Tables are described by name and generator up front but only synthesised
        when the bank is first used, so listing the names for a parameter costs
        nothing. The whole bank is built once per process and shared by every
        plugin instance - it is immutable, so sharing is free and safe.
    */
    class WavetableBank
    {
    public:
        /** The shared bank. Building it takes a moment the first time. */
        static const WavetableBank& factory();

        /** Table names, available without building anything. */
        static juce::StringArray getTableNames();

        static int getNumTables();

        const Wavetable& getTable (int index) const noexcept;

        /** Four exact basic shapes for the sub oscillator, addressed by frame
            rather than morphed: sine, triangle, saw, square. */
        const Wavetable& getSubTable() const noexcept { return subTable; }

    private:
        WavetableBank();

        std::vector<Wavetable> tables;
        Wavetable subTable;
    };
}
