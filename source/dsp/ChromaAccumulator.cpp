#include "ChromaAccumulator.h"

#include <algorithm>
#include <cmath>

#include "DspCommon.h"

namespace keyy
{

namespace
{
    /** Halbe Breite der Umgebung für das örtliche Mittel, in FFT-Fächern. */
    constexpr int localWidth = 24;

    /** Kürzeste Datei, die überhaupt analysiert wird (bei 11 kHz knapp 0,1 s). */
    constexpr size_t minimumSamples = 1024;
}

void ChromaAccumulator::prepare (double newSampleRate, const Settings& newSettings)
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
    prefix.assign (static_cast<size_t> (n / 2 + 2), 0.0);

    reset();
}

void ChromaAccumulator::reset()
{
    pending.clear();
    peaks.clear();
    fine.fill (0.0);
    cents.fill (0.0);
    numFrames = 0;
}

void ChromaAccumulator::process (const float* samples, int numSamples)
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

void ChromaAccumulator::flush()
{
    if (fft != nullptr && numFrames == 0 && pending.size() >= minimumSamples)
    {
        pending.resize (static_cast<size_t> (fft->getSize()), 0.0f);
        analyseFrame (pending.data());
    }

    pending.clear();
}

void ChromaAccumulator::analyseFrame (const float* samples)
{
    const int n = fft->getSize();
    const int half = n / 2;

    double energy = 0.0;
    for (int i = 0; i < n; ++i)
    {
        energy += static_cast<double> (samples[i]) * samples[i];
        frame[static_cast<size_t> (i)] = samples[i] * window[static_cast<size_t> (i)];
    }

    if (std::sqrt (energy / n) < settings.silenceRms)
        return;

    fft->magnitudes (frame.data(), magnitudes.data());

    prefix[0] = 0.0;
    for (int k = 0; k <= half; ++k)
        prefix[static_cast<size_t> (k + 1)] = prefix[static_cast<size_t> (k)] + magnitudes[static_cast<size_t> (k)];

    const double binHz = sampleRate / n;
    const int kMin = std::max (2, static_cast<int> (std::ceil (settings.minHz / binHz)));
    const int kMax = std::min (half - 2, static_cast<int> (std::floor (settings.maxHz / binHz)));

    if (kMax <= kMin)
        return;

    const auto mag = [this] (int k) { return magnitudes[static_cast<size_t> (k)]; };

    float maxMagnitude = 0.0f;
    for (int k = kMin; k <= kMax; ++k)
        maxMagnitude = std::max (maxMagnitude, mag (k));

    if (maxMagnitude <= 0.0f)
        return;

    // Alles unter -80 dB relativ zur lautesten Spitze ist Rechenrauschen.
    const float floorMagnitude = maxMagnitude * 1.0e-4f;

    peaks.clear();
    double total = 0.0;

    for (int k = kMin; k <= kMax; ++k)
    {
        const float m = mag (k);

        if (m <= mag (k - 1) || m < mag (k + 1) || m < floorMagnitude)
            continue;

        const int lo = std::max (0, k - localWidth);
        const int hi = std::min (half, k + localWidth);
        const double localMean = (prefix[static_cast<size_t> (hi + 1)] - prefix[static_cast<size_t> (lo)]) / (hi - lo + 1);

        if (m < settings.peakThreshold * localMean)
            continue;

        // Parabel durch die logarithmierten Beträge der drei Fächer: liefert
        // Frequenz und Höhe der Spitze zwischen den Fächern. Ohne das wäre
        // die Tonhöhe bei tiefen Tönen nur auf einen Viertelton genau — zu
        // grob für den Stimmton.
        const double a = std::log (std::max (mag (k - 1), 1.0e-12f));
        const double b = std::log (m);
        const double c = std::log (std::max (mag (k + 1), 1.0e-12f));
        const double denominator = a - 2.0 * b + c;
        const double delta = denominator < 0.0 ? std::clamp (0.5 * (a - c) / denominator, -0.5, 0.5) : 0.0;
        const double peakLog = b - 0.25 * (a - c) * delta;

        const double hz = (k + delta) * binHz;
        const double weight = std::exp (peakLog * settings.magnitudeExponent);

        peaks.push_back ({ midiFromHz (hz), weight });
        total += weight;
    }

    if (total <= 0.0)
        return;

    double frameWeight = 1.0;
    if (settings.tonalityWeighting > 0.0)
    {
        double spectrum = 0.0;
        for (int k = kMin; k <= kMax; ++k)
            spectrum += std::pow (static_cast<double> (mag (k)), settings.magnitudeExponent);

        if (spectrum > 0.0)
            frameWeight = std::pow (std::min (1.0, total / spectrum), settings.tonalityWeighting);
    }

    for (const auto& peak : peaks)
    {
        const double w = peak.weight / total * frameWeight;

        // Feines Tonklassen-Histogramm, linear auf die beiden Nachbarfächer verteilt.
        const double x = wrap (peak.pitch * binsPerSemitone, static_cast<double> (fineBins));
        const int i0 = static_cast<int> (x);
        const double fx = x - i0;
        fine[static_cast<size_t> (i0 % fineBins)]       += w * (1.0 - fx);
        fine[static_cast<size_t> ((i0 + 1) % fineBins)] += w * fx;

        // Abweichung vom 440-Hz-Raster in Cent, 0..100 im Kreis.
        const double deviation = wrap (peak.pitch * 100.0, static_cast<double> (centBins));
        const int j0 = static_cast<int> (deviation);
        const double fd = deviation - j0;
        cents[static_cast<size_t> (j0 % centBins)]       += w * (1.0 - fd);
        cents[static_cast<size_t> ((j0 + 1) % centBins)] += w * fd;
    }

    ++numFrames;
}

double ChromaAccumulator::getTuningCents() const
{
    double total = 0.0;
    for (const auto v : cents)
        total += v;

    if (total <= 0.0)
        return 0.0;

    // Glätten mit einem Dreieck über ±6 Cent, im Kreis: -50 und +49 Cent
    // sind Nachbarn.
    std::array<double, centBins> smooth {};
    for (int i = 0; i < centBins; ++i)
        for (int d = -6; d <= 6; ++d)
            smooth[static_cast<size_t> (i)] += cents[static_cast<size_t> (wrap (i + d, centBins))] * (7 - std::abs (d));

    int best = 0;
    for (int i = 1; i < centBins; ++i)
        if (smooth[static_cast<size_t> (i)] > smooth[static_cast<size_t> (best)])
            best = i;

    const double a = smooth[static_cast<size_t> (wrap (best - 1, centBins))];
    const double b = smooth[static_cast<size_t> (best)];
    const double c = smooth[static_cast<size_t> (wrap (best + 1, centBins))];
    const double denominator = a - 2.0 * b + c;
    const double delta = denominator < 0.0 ? 0.5 * (a - c) / denominator : 0.0;

    double result = wrap (best + delta, static_cast<double> (centBins));
    if (result >= 50.0)
        result -= 100.0;

    return result;
}

std::array<double, 12> ChromaAccumulator::getChroma() const
{
    std::array<double, 12> chroma {};
    const double shift = getTuningCents() / 100.0;

    // Jedes feine Fach um den Stimmton verschieben und linear auf die beiden
    // nächsten Halbtöne verteilen. Ein Ton genau im korrigierten Raster
    // landet damit vollständig in seiner Tonklasse.
    for (int i = 0; i < fineBins; ++i)
    {
        const double position = static_cast<double> (i) / binsPerSemitone - shift;
        const double base = std::floor (position);
        const double fraction = position - base;
        const int semitone = static_cast<int> (base);
        const double value = fine[static_cast<size_t> (i)];

        chroma[static_cast<size_t> (wrap (semitone, 12))]     += value * (1.0 - fraction);
        chroma[static_cast<size_t> (wrap (semitone + 1, 12))] += value * fraction;
    }

    double total = 0.0;
    for (const auto v : chroma)
        total += v;

    if (total > 0.0)
        for (auto& v : chroma)
            v /= total;

    return chroma;
}

} // namespace keyy
