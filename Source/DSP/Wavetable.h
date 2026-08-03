#pragma once

#include <array>
#include <complex>
#include <functional>
#include <vector>

#include <juce_core/juce_core.h>

namespace nog::dsp
{
    /**
        A band-limited wavetable: a stack of single-cycle frames that an
        oscillator sweeps through.

        Every table is built from **harmonic spectra** rather than from raw
        samples, because that is what makes band-limiting exact. For each frame
        the builder is handed a list of harmonic amplitudes; it then synthesises
        one copy of the frame per mip level, each containing only the harmonics
        that still fit below Nyquist at that level's frequency range. Playing a
        high note picks a sparse mip and simply has no harmonics left to alias.

        Level 0 is the full-bandwidth version; each level after it keeps half as
        many harmonics. Sizes shrink alongside the harmonic count until they hit
        minMipSize, so the whole pyramid costs about two and a half frames.
    */
    class Wavetable
    {
    public:
        static constexpr int frameSize    = 2048;
        static constexpr int frameOrder   = 11;     // 2^11 == frameSize
        static constexpr int numMipLevels = 9;

        /** Mips stop shrinking here even though their harmonic content keeps
            falling. Reading a table with linear interpolation is only accurate
            when there are plenty of points per cycle of the highest harmonic;
            an 8-point table band-limited to 4 harmonics is correct in theory and
            audibly rough in practice. Holding the floor at 256 samples costs
            about 25% more memory and removes the problem. */
        static constexpr int minMipSize = 256;

        /** Number of samples stored at a mip level. */
        static constexpr int mipSize (int level) noexcept
        {
            // Written out rather than using juce::jlimit, which is not constexpr.
            const auto clamped = level < 0 ? 0
                               : (level > numMipLevels - 1 ? numMipLevels - 1 : level);
            const auto shrunk  = frameSize >> clamped;

            return shrunk < minMipSize ? minMipSize : shrunk;
        }

        using Spectrum = std::vector<std::complex<float>>;

        Wavetable() = default;

        /** Builds the table. @p frameSpectra is one spectrum per frame, indexed
            by harmonic number - element 0 is DC and is ignored, element 1 is the
            fundamental. Amplitudes need not be normalised. */
        void build (juce::String tableName, const std::vector<Spectrum>& frameSpectra);

        bool isEmpty() const noexcept { return frames.empty(); }
        int getNumFrames() const noexcept { return static_cast<int> (frames.size()); }
        const juce::String& getName() const noexcept { return name; }

        /** The coarsest mip level that is still free of aliasing at a phase
            increment of @p increment cycles per sample. */
        static int mipForIncrement (double increment) noexcept;

        /** Analyses one cycle of a waveform into harmonics, so that table
            generators can be written as ordinary time-domain formulas.

            The waveform is sampled several times more densely than the frame
            size and then truncated, which keeps a discontinuous shape - a saw,
            a pulse, a sync sweep - from folding energy back down into the
            harmonics that are kept.

            @p oneCycle is called with a phase in [0, 1).
        */
        static Spectrum analyse (const std::function<float (float)>& oneCycle);

        /** Builds a spectrum from a sine series: harmonic @p n gets amplitude
            `amplitude (n)`. The natural way to describe saws, squares, organ
            drawbars and formant shapes. */
        static Spectrum sineSeries (const std::function<float (int)>& amplitude);

        /** Highest harmonic a spectrum from this class carries. */
        static constexpr int maxHarmonic = frameSize / 2;

        /** One sample, interpolated within the frame and between adjacent
            frames. @p framePosition is a fractional frame index. */
        float getSample (float framePosition, int mipLevel, double phase) const noexcept;

    private:
        /** Synthesises one mip of one frame by inverse-transforming the
            harmonics that fit at that level. */
        static std::vector<float> renderMip (const Spectrum& spectrum, int mipLevel);

        static float sampleMip (const std::vector<float>& mip, double phase) noexcept;

        juce::String name;

        // frames[frame][mip]
        std::vector<std::array<std::vector<float>, numMipLevels>> frames;
    };
}
