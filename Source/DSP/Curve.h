#pragma once

#include <cmath>

#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        Bends a 0..1 ramp.

        Negative is slow-then-fast, positive is fast-then-slow, and zero passes
        the ramp through untouched. The power law is chosen so that the control
        feels even across its range: at ±1 the exponent is 4 and 1/4, which is
        about as far as a shape can bend before it stops being useful.

        Shared rather than duplicated because the envelopes and the modulation
        matrix want exactly the same curve. A modulation routing shaped by a
        different function from the envelope driving it would be a surprise
        every time someone compared the two.
    */
    inline float shapeCurve (float x, float curve) noexcept
    {
        if (std::abs (curve) < 1.0e-4f)
            return x;

        const auto exponent = std::pow (4.0f, -curve);
        return std::pow (juce::jlimit (0.0f, 1.0f, x), exponent);
    }
}
