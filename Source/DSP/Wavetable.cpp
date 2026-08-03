#include "DSP/Wavetable.h"

#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace nog::dsp
{
    namespace
    {
        /** Harmonics kept at a mip level.

            Halves with every level, which is what makes the pyramid work, but
            is also capped by what the stored size can physically represent
            once the size stops shrinking at minMipSize.
        */
        int harmonicLimitForMip (int mipLevel) noexcept
        {
            const auto wanted   = Wavetable::frameSize >> (mipLevel + 1);
            const auto storable = Wavetable::mipSize (mipLevel) / 2 - 1;

            return juce::jmax (1, juce::jmin (wanted, storable));
        }

        /** log2 of a power-of-two size. */
        int orderForSize (int size) noexcept
        {
            auto order = 0;

            while ((1 << order) < size)
                ++order;

            return order;
        }

        /** Transforms are cached per order: building a juce::dsp::FFT computes
            twiddle tables, and a full factory bank needs thousands of them.
            FFT::perform is const and re-entrant, so one instance per order is
            enough. */
        // Analysis runs at a higher order than the frame itself, so the cache
        // has to reach beyond frameOrder.
        constexpr int maxCachedOrder = Wavetable::frameOrder + 3;

        const juce::dsp::FFT& transformForOrder (int order)
        {
            static std::array<std::unique_ptr<juce::dsp::FFT>, maxCachedOrder + 1> cache;
            static juce::SpinLock lock;

            const auto index = static_cast<size_t> (juce::jlimit (1, maxCachedOrder, order));

            const juce::SpinLock::ScopedLockType scoped (lock);

            if (cache[index] == nullptr)
                cache[index] = std::make_unique<juce::dsp::FFT> (static_cast<int> (index));

            return *cache[index];
        }
    }

    int Wavetable::mipForIncrement (double increment) noexcept
    {
        if (increment <= 0.0)
            return 0;

        // The highest harmonic that still fits below Nyquist.
        const auto affordableHarmonics = 0.5 / increment;

        // Level m keeps frameSize / 2^(m+1) harmonics; pick the smallest m
        // whose count does not exceed what fits below Nyquist.
        const auto required = static_cast<double> (frameSize) / (2.0 * affordableHarmonics);
        const auto level    = static_cast<int> (std::ceil (std::log2 (juce::jmax (1.0, required))));

        return juce::jlimit (0, numMipLevels - 1, level);
    }

    Wavetable::Spectrum Wavetable::analyse (const std::function<float (float)>& oneCycle)
    {
        // Four times the frame size. Anything the waveform puts above harmonic
        // 1024 lands in bins that are discarded rather than folding down into
        // the ones that are kept.
        constexpr int oversampling = 4;
        constexpr int size  = frameSize * oversampling;
        constexpr int order = frameOrder + 2;

        std::vector<std::complex<float>> input (static_cast<size_t> (size));
        std::vector<std::complex<float>> output (static_cast<size_t> (size));

        for (int i = 0; i < size; ++i)
            input[static_cast<size_t> (i)] = { oneCycle (static_cast<float> (i) / static_cast<float> (size)), 0.0f };

        transformForOrder (order).perform (input.data(), output.data(), false);

        Spectrum spectrum (static_cast<size_t> (maxHarmonic) + 1, { 0.0f, 0.0f });

        const auto scale = 1.0f / static_cast<float> (size);

        for (int harmonic = 1; harmonic <= maxHarmonic; ++harmonic)
            spectrum[static_cast<size_t> (harmonic)] = output[static_cast<size_t> (harmonic)] * scale;

        return spectrum;
    }

    Wavetable::Spectrum Wavetable::sineSeries (const std::function<float (int)>& amplitude)
    {
        Spectrum spectrum (static_cast<size_t> (maxHarmonic) + 1, { 0.0f, 0.0f });

        // A purely imaginary coefficient produces a sine rather than a cosine,
        // which is the phase convention every classic waveform is written in.
        for (int harmonic = 1; harmonic <= maxHarmonic; ++harmonic)
            spectrum[static_cast<size_t> (harmonic)] = { 0.0f, -amplitude (harmonic) };

        return spectrum;
    }

    std::vector<float> Wavetable::renderMip (const Spectrum& spectrum, int mipLevel)
    {
        const auto size  = mipSize (mipLevel);
        const auto order = orderForSize (size);
        const auto limit = juce::jmin (harmonicLimitForMip (mipLevel),
                                       static_cast<int> (spectrum.size()) - 1);

        std::vector<std::complex<float>> input (static_cast<size_t> (size), { 0.0f, 0.0f });

        // A real output needs a conjugate-symmetric spectrum, so each harmonic
        // is mirrored into the upper half of the buffer.
        for (int harmonic = 1; harmonic <= limit; ++harmonic)
        {
            const auto value = spectrum[static_cast<size_t> (harmonic)];

            input[static_cast<size_t> (harmonic)]        = value;
            input[static_cast<size_t> (size - harmonic)] = std::conj (value);
        }

        std::vector<std::complex<float>> output (static_cast<size_t> (size));

        transformForOrder (order).perform (input.data(), output.data(), true);

        std::vector<float> samples (static_cast<size_t> (size));

        // The inverse transform divides by its own length, so without this a
        // small mip would come out far louder than a large one and the level
        // would jump every time a note crossed a mip boundary.
        const auto scale = static_cast<float> (size);

        for (size_t i = 0; i < samples.size(); ++i)
            samples[i] = output[i].real() * scale;

        return samples;
    }

    void Wavetable::build (juce::String tableName, const std::vector<Spectrum>& frameSpectra)
    {
        name = std::move (tableName);
        frames.clear();

        if (frameSpectra.empty())
            return;

        frames.resize (frameSpectra.size());

        for (size_t frameIndex = 0; frameIndex < frameSpectra.size(); ++frameIndex)
        {
            auto& frame = frames[frameIndex];

            for (int mip = 0; mip < numMipLevels; ++mip)
                frame[static_cast<size_t> (mip)] = renderMip (frameSpectra[frameIndex], mip);

            // Normalise every frame to the same peak so that sweeping the table
            // does not change the level. The scale is taken from the full
            // bandwidth mip and applied to all of them, so switching mip level
            // mid-note cannot produce a step in amplitude.
            auto peak = 0.0f;

            for (const auto sample : frame[0])
                peak = juce::jmax (peak, std::abs (sample));

            if (peak > 1.0e-6f)
            {
                const auto scale = 1.0f / peak;

                for (auto& mip : frame)
                    for (auto& sample : mip)
                        sample *= scale;
            }
        }
    }

    float Wavetable::sampleMip (const std::vector<float>& mip, double phase) noexcept
    {
        const auto size = static_cast<int> (mip.size());

        if (size <= 0)
            return 0.0f;

        const auto position = phase * static_cast<double> (size);
        const auto index    = static_cast<int> (position) % size;
        const auto fraction = static_cast<float> (position - std::floor (position));

        // Catmull-Rom rather than linear. Reading a 256-point table sparsely -
        // which is what a high note does - leaves linear interpolation with
        // roughly 0.5% error, audible as a buzz around -47 dB. The cubic costs
        // a handful of extra multiplies and drops that below -90 dB.
        const auto at = [&mip, size] (int i) noexcept
        {
            return mip[static_cast<size_t> (((i % size) + size) % size)];
        };

        const auto x0 = at (index - 1);
        const auto x1 = at (index);
        const auto x2 = at (index + 1);
        const auto x3 = at (index + 2);

        const auto a = -0.5f * x0 + 1.5f * x1 - 1.5f * x2 + 0.5f * x3;
        const auto b =         x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
        const auto c = -0.5f * x0                + 0.5f * x2;

        return ((a * fraction + b) * fraction + c) * fraction + x1;
    }

    float Wavetable::getSample (float framePosition, int mipLevel, double phase) const noexcept
    {
        if (frames.empty())
            return 0.0f;

        const auto frameCount = static_cast<int> (frames.size());
        const auto mip        = static_cast<size_t> (juce::jlimit (0, numMipLevels - 1, mipLevel));

        const auto clamped = juce::jlimit (0.0f, static_cast<float> (frameCount - 1), framePosition);
        const auto lower   = static_cast<int> (clamped);
        const auto upper   = juce::jmin (lower + 1, frameCount - 1);
        const auto blend   = clamped - static_cast<float> (lower);

        const auto a = sampleMip (frames[static_cast<size_t> (lower)][mip], phase);

        if (lower == upper || blend < 1.0e-4f)
            return a;

        const auto b = sampleMip (frames[static_cast<size_t> (upper)][mip], phase);

        return a + (b - a) * blend;
    }
}
