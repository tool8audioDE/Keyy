#include "TempoEstimator.h"

#include <algorithm>
#include <cmath>

#include "DspCommon.h"

namespace keyy
{

void TempoEstimator::prepare (double newSampleRate, const Settings& newSettings)
{
    sampleRate = newSampleRate;
    settings   = newSettings;
    settings.hopSize = std::clamp (settings.hopSize, 1, 1 << settings.fftOrder);

    fft = std::make_unique<Fft> (settings.fftOrder);
    const int n = fft->getSize();

    window.resize (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        window[static_cast<size_t> (i)] = static_cast<float> (0.5 - 0.5 * std::cos (2.0 * pi * i / n));

    frame.assign (static_cast<size_t> (n), 0.0f);
    magnitudes.assign (static_cast<size_t> (n / 2 + 1), 0.0f);
    previous.assign (static_cast<size_t> (n / 2 + 1), 0.0f);

    reset();
}

void TempoEstimator::reset()
{
    pending.clear();
    envelope.clear();
    std::fill (previous.begin(), previous.end(), 0.0f);
    hasPrevious = false;
}

void TempoEstimator::process (const float* samples, int numSamples)
{
    if (fft == nullptr || numSamples <= 0)
        return;

    pending.insert (pending.end(), samples, samples + numSamples);

    const auto n   = static_cast<size_t> (fft->getSize());
    const auto hop = static_cast<size_t> (settings.hopSize);

    size_t start = 0;
    while (pending.size() - start >= n)
    {
        analyseFrame (pending.data() + start);
        start += hop;
    }

    if (start > 0)
        pending.erase (pending.begin(), pending.begin() + static_cast<std::ptrdiff_t> (start));
}

void TempoEstimator::analyseFrame (const float* samples)
{
    const int n = fft->getSize();

    for (int i = 0; i < n; ++i)
        frame[static_cast<size_t> (i)] = samples[i] * window[static_cast<size_t> (i)];

    fft->magnitudes (frame.data(), magnitudes.data());

    // Logarithmisch verdichtet, damit ein leiser Hi-Hat-Einsatz neben einer
    // lauten Kick nicht verschwindet.
    double flux = 0.0;
    for (int k = 1; k <= n / 2; ++k)
    {
        const auto index = static_cast<size_t> (k);
        const float value = std::log1p (100.0f * magnitudes[index]);

        if (hasPrevious && value > previous[index])
            flux += value - previous[index];

        previous[index] = value;
    }

    hasPrevious = true;
    envelope.push_back (static_cast<float> (flux));
}

TempoEstimator::Result TempoEstimator::estimate (double durationSeconds) const
{
    Result result;

    const double rate = getEnvelopeRate();
    const int count = static_cast<int> (envelope.size());

    if (count < static_cast<int> (rate))     // unter einer Sekunde
        return result;

    // Gleitenden Mittelwert (±0,25 s) abziehen und halbwellengleichrichten:
    // übrig bleiben die Einsätze, nicht der allgemeine Pegel.
    std::vector<double> prefix (static_cast<size_t> (count + 1), 0.0);
    for (int i = 0; i < count; ++i)
        prefix[static_cast<size_t> (i + 1)] = prefix[static_cast<size_t> (i)] + envelope[static_cast<size_t> (i)];

    const int radius = std::max (1, static_cast<int> (rate * 0.25));
    std::vector<double> onsets (static_cast<size_t> (count));
    double mean = 0.0;

    for (int i = 0; i < count; ++i)
    {
        const int lo = std::max (0, i - radius);
        const int hi = std::min (count - 1, i + radius);
        const double local = (prefix[static_cast<size_t> (hi + 1)] - prefix[static_cast<size_t> (lo)]) / (hi - lo + 1);
        const double v = std::max (0.0, envelope[static_cast<size_t> (i)] - local);
        onsets[static_cast<size_t> (i)] = v;
        mean += v;
    }

    mean /= count;
    if (mean <= 0.0)
        return result;

    {
        std::vector<double> sorted (onsets);
        const auto top = std::max<size_t> (1, sorted.size() / 20);
        std::nth_element (sorted.begin(), sorted.end() - static_cast<std::ptrdiff_t> (top), sorted.end());

        double topSum = 0.0;
        for (auto it = sorted.end() - static_cast<std::ptrdiff_t> (top); it != sorted.end(); ++it)
            topSum += *it;

        result.salience = topSum / static_cast<double> (top) / mean;
    }

    // Gehaltene Töne schweben gegeneinander, und die Schwebung ist
    // tatsächlich periodisch — die Autokorrelation fände darin ein "Tempo".
    // Schwebungen sind aber weich, Schläge spitz.
    if (result.salience < settings.minSalience)
        return result;

    for (auto& v : onsets)
        v -= mean;

    // Autokorrelation bis zum vierfachen Schlagabstand des langsamsten
    // Tempos, höchstens aber bis zur halben Länge — darüber stützt sich
    // jeder Wert auf zu wenige Paare.
    const int maxLag = std::min (count / 2, static_cast<int> (std::ceil (rate * 60.0 / settings.minBpm * 4.0)) + 2);
    if (maxLag < 4)
        return result;

    std::vector<double> acf (static_cast<size_t> (maxLag + 1), 0.0);
    for (int lag = 0; lag <= maxLag; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i + lag < count; ++i)
            sum += onsets[static_cast<size_t> (i)] * onsets[static_cast<size_t> (i + lag)];

        acf[static_cast<size_t> (lag)] = sum / (count - lag);
    }

    if (acf[0] <= 0.0)
        return result;

    const auto acfAt = [&] (double lag)
    {
        const int i = static_cast<int> (lag);
        const double f = lag - i;
        return acf[static_cast<size_t> (i)] * (1.0 - f) + acf[static_cast<size_t> (i + 1)] * f;
    };

    const auto comb = [&] (double bpm, int multiples)
    {
        const double beat = rate * 60.0 / bpm;
        double sum = 0.0;
        int used = 0;

        for (int k = 1; k <= multiples; ++k)
        {
            const double lag = k * beat;
            if (lag >= maxLag)
                break;

            sum += acfAt (lag);
            ++used;
        }

        return used > 0 ? sum / used : 0.0;
    };

    double bestBpm = 0.0, bestScore = -1.0e300, bestRaw = 0.0;

    for (double bpm = settings.minBpm; bpm <= settings.maxBpm; bpm += 0.05)
    {
        const double octaves = std::log2 (bpm / settings.preferredBpm);
        const double prior = std::exp (-0.5 * octaves * octaves);
        const double raw = comb (bpm, 4);
        const double score = raw * prior;

        if (score > bestScore)
        {
            bestScore = score;
            bestBpm = bpm;
            bestRaw = raw;
        }
    }

    // Kein wiederkehrendes Muster: Flächen, Pads, Stille.
    if (bestRaw < 0.1 * acf[0])
        return result;

    // Verfeinern mit längerem Kamm: Acht Schläge Abstand messen das Tempo
    // achtmal genauer als einer.
    double refined = bestBpm, refinedScore = -1.0e300;
    for (double bpm = bestBpm - 0.5; bpm <= bestBpm + 0.5; bpm += 0.005)
    {
        const double score = comb (bpm, 8);
        if (score > refinedScore)
        {
            refinedScore = score;
            refined = bpm;
        }
    }

    result.valid = true;
    result.onsetBpm = refined;
    result.bpm = refined;

    if (durationSeconds > 0.0 && durationSeconds <= settings.maxLoopSeconds)
    {
        for (int beats = 4; beats <= 1024; beats *= 2)
        {
            const double loopBpm = 60.0 * beats / durationSeconds;

            if (std::abs (loopBpm - refined) / refined > 0.04)
                continue;

            if (std::abs (loopBpm - std::round (loopBpm)) > 0.02)
                continue;

            result.bpm = std::round (loopBpm);
            result.fromLoopLength = true;
            break;
        }
    }

    return result;
}

} // namespace keyy
