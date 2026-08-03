#include "Modulation/ModMatrix.h"

namespace nog
{
    ModMatrix::ModMatrix()
    {
        routings.reserve (static_cast<size_t> (ids::numMatrixSlots));
    }

    void ModMatrix::refresh (const ParameterStore& parameters)
    {
        routings.clear();

        for (int i = 0; i < ids::numMatrixSlots; ++i)
        {
            const auto& slot = parameters.matrix[static_cast<size_t> (i)];

            if (slot.enabled == nullptr || ! slot.enabled->get())
                continue;

            const auto amount = slot.amount->get();

            // Skip slots that would contribute nothing; this is the common case,
            // since most of the sixteen slots are empty in a typical patch.
            if (std::abs (amount) < 1.0e-5f)
                continue;

            const auto source = static_cast<mod::Source> (slot.source->getIndex());
            const auto dest   = static_cast<mod::Dest>   (slot.dest->getIndex());

            if (source == mod::Source::None || dest == mod::Dest::None)
                continue;

            routings.push_back ({ source, dest, amount, slot.bipolar->get() });
        }

        for (int i = 0; i < ids::numMacros; ++i)
            if (auto* macro = parameters.macro[static_cast<size_t> (i)])
                macros[static_cast<size_t> (i)] = macro->get();
    }

    void ModMatrix::applyGlobalSources (ModulationFrame& frame) const noexcept
    {
        frame.setSource (mod::Source::ModWheel,   modWheel);
        frame.setSource (mod::Source::PitchBend,  pitchBend);
        frame.setSource (mod::Source::Aftertouch, aftertouch);

        frame.setSource (mod::Source::Macro1, macros[0]);
        frame.setSource (mod::Source::Macro2, macros[1]);
        frame.setSource (mod::Source::Macro3, macros[2]);
        frame.setSource (mod::Source::Macro4, macros[3]);
    }

    void ModMatrix::apply (ModulationFrame& frame) const noexcept
    {
        for (const auto& routing : routings)
        {
            auto value = frame.getSource (routing.source);

            // The bipolar switch re-centres a 0..1 source around zero, so an
            // envelope can push a destination down as well as up.
            if (routing.bipolar)
                value = value * 2.0f - 1.0f;

            frame.offsets[static_cast<size_t> (routing.dest)] += value * routing.amount;
        }
    }

    float ModMatrix::getModulationDepth (mod::Dest dest) const noexcept
    {
        auto depth = 0.0f;

        for (const auto& routing : routings)
            if (routing.dest == dest)
                depth += std::abs (routing.amount);

        return depth;
    }

    void ModMatrix::setMacro (int index, float value) noexcept
    {
        if (juce::isPositiveAndBelow (index, ids::numMacros))
            macros[static_cast<size_t> (index)] = value;
    }
}
