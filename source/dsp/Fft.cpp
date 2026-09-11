#include "Fft.h"

#include <utility>

#include "DspCommon.h"

namespace keyy
{

Fft::Fft (int order)
    : size (1 << order)
{
    const auto n = static_cast<size_t> (size);

    twiddles.resize (n / 2);
    for (size_t k = 0; k < n / 2; ++k)
    {
        const double angle = -2.0 * pi * static_cast<double> (k) / static_cast<double> (size);
        twiddles[k] = { static_cast<float> (std::cos (angle)), static_cast<float> (std::sin (angle)) };
    }

    bitReversed.resize (n);
    for (int i = 0; i < size; ++i)
    {
        int reversed = 0;
        for (int bit = 0; bit < order; ++bit)
            if (i & (1 << bit))
                reversed |= 1 << (order - 1 - bit);

        bitReversed[static_cast<size_t> (i)] = reversed;
    }

    work.resize (n);
}

void Fft::perform (std::complex<float>* data) const noexcept
{
    for (int i = 0; i < size; ++i)
    {
        const int j = bitReversed[static_cast<size_t> (i)];
        if (i < j)
            std::swap (data[i], data[j]);
    }

    for (int length = 2; length <= size; length <<= 1)
    {
        const int half = length / 2;
        const int step = size / length;

        for (int start = 0; start < size; start += length)
        {
            for (int k = 0; k < half; ++k)
            {
                const auto t = twiddles[static_cast<size_t> (k * step)] * data[start + k + half];
                data[start + k + half] = data[start + k] - t;
                data[start + k] += t;
            }
        }
    }
}

void Fft::magnitudes (const float* input, float* magnitudesOut)
{
    for (int i = 0; i < size; ++i)
        work[static_cast<size_t> (i)] = { input[i], 0.0f };

    perform (work.data());

    for (int k = 0; k <= size / 2; ++k)
        magnitudesOut[k] = std::abs (work[static_cast<size_t> (k)]);
}

} // namespace keyy
