#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "dsp/DspCommon.h"
#include "dsp/Key.h"
#include "dsp/KeyAnalyzer.h"

namespace keyy::test
{

/** Harmonischer Ton mit abfallenden Obertönen und kurzen Rampen.

    Obertöne, weil echte Instrumente welche haben und genau die das
    Chromagramm verfälschen: Der dritte Oberton eines C ist ein G. Eine
    Erkennung, die nur an reinen Sinustönen funktioniert, taugt nichts.
*/
inline void addTone (std::vector<float>& out, double sampleRate, double hz,
                     size_t start, size_t length, float amplitude, int harmonics = 6)
{
    const size_t end  = std::min (out.size(), start + length);
    const auto   fade = static_cast<size_t> (0.01 * sampleRate);

    for (int h = 1; h <= harmonics; ++h)
    {
        const double f = hz * h;
        if (f >= 0.45 * sampleRate)
            break;

        const double gain = amplitude / h;
        const double w = 2.0 * pi * f / sampleRate;
        const double c = std::cos (w), s = std::sin (w);
        double re = 1.0, im = 0.0;

        // Drehzeiger statt sin je Sample: gleich genau, ein Vielfaches schneller.
        for (size_t n = start; n < end; ++n)
        {
            const size_t fromStart = n - start, toEnd = end - n;
            const double envelope = std::min ({ 1.0, static_cast<double> (fromStart) / fade, static_cast<double> (toEnd) / fade });

            out[n] += static_cast<float> (gain * envelope * im);

            const double nextRe = re * c - im * s;
            im = re * s + im * c;
            re = nextRe;
        }
    }
}

/** Kadenz I-IV-V-I in Dur, i-iv-V-i in Moll (mit Leitton), mit Basston.

    Die Akkorde liegen zwischen A3 und G#4, der Bass eine Oktave darunter —
    ungefähr dort, wo Keys und Pads in einem Beat sitzen.
*/
inline std::vector<float> cadence (const Key& key, double sampleRate, double secondsPerChord = 1.0,
                                   double referenceA = 440.0, int repeats = 2)
{
    struct Chord { int degree; bool minorTriad; };

    const std::vector<Chord> chords = key.mode == Mode::Major
        ? std::vector<Chord> { { 0, false }, { 5, false }, { 7, false }, { 0, false } }
        : std::vector<Chord> { { 0, true  }, { 5, true  }, { 7, false }, { 0, true  } };

    const auto chordLength = static_cast<size_t> (secondsPerChord * sampleRate);
    std::vector<float> out (chordLength * chords.size() * static_cast<size_t> (repeats), 0.0f);

    size_t position = 0;
    for (int r = 0; r < repeats; ++r)
    {
        for (const auto& chord : chords)
        {
            const int pitchClass = wrap (key.tonic + chord.degree, 12);
            const int root = 57 + wrap (pitchClass - 9, 12);
            const int third = chord.minorTriad ? 3 : 4;

            for (const int interval : { 0, third, 7 })
                addTone (out, sampleRate, hzFromMidi (root + interval, referenceA), position, chordLength, 0.15f);

            addTone (out, sampleRate, hzFromMidi (root - 12, referenceA), position, chordLength, 0.2f);
            position += chordLength;
        }
    }

    return out;
}

/** Gleichbleibendes Rauschen für Schlaginstrumente, ohne Zufall aus der Laufzeitbibliothek. */
struct Noise
{
    uint32_t state = 12345u;

    float next() noexcept
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float> (state >> 8) / 8388608.0f - 1.0f;
    }
};

/** Kurzer Schlag: abklingender Ton plus Rauschen. */
inline void addHit (std::vector<float>& out, double sampleRate, size_t start, double hz,
                    float toneLevel, float noiseLevel, double decaySeconds, Noise& noise)
{
    const auto length = static_cast<size_t> (decaySeconds * 5.0 * sampleRate);
    const double w = 2.0 * pi * hz / sampleRate;

    for (size_t i = 0; i < length && start + i < out.size(); ++i)
    {
        const double envelope = std::exp (-static_cast<double> (i) / (decaySeconds * sampleRate));
        const double tone = std::sin (w * static_cast<double> (i));
        out[start + i] += static_cast<float> (envelope * (toneLevel * tone + noiseLevel * noise.next()));
    }
}

/** Klick auf jedem Schlag. */
inline std::vector<float> clickTrack (double bpm, double sampleRate, double seconds)
{
    std::vector<float> out (static_cast<size_t> (seconds * sampleRate), 0.0f);
    Noise noise;
    const double period = 60.0 / bpm * sampleRate;

    for (double t = 0.0; t < static_cast<double> (out.size()); t += period)
        addHit (out, sampleRate, static_cast<size_t> (t), 1000.0, 0.5f, 0.3f, 0.01, noise);

    return out;
}

/** Drum-Loop: Kick auf 1 und 3, Snare auf 2 und 4, Hi-Hat auf Achteln.

    @param extraSeconds  Stille am Ende — ein nicht exakt geschnittener Loop
*/
inline std::vector<float> drumLoop (double bpm, int bars, double sampleRate, double extraSeconds = 0.0)
{
    const double beatSamples = 60.0 / bpm * sampleRate;
    const auto length = static_cast<size_t> (std::llround (beatSamples * 4 * bars + extraSeconds * sampleRate));
    std::vector<float> out (length, 0.0f);
    Noise noise;

    for (int eighth = 0; eighth < bars * 8; ++eighth)
    {
        const auto start = static_cast<size_t> (std::llround (eighth * beatSamples * 0.5));
        const int inBar = eighth % 8;

        if (inBar == 0 || inBar == 4)
            addHit (out, sampleRate, start, 55.0, 0.8f, 0.05f, 0.08, noise);
        else if (inBar == 2 || inBar == 6)
            addHit (out, sampleRate, start, 200.0, 0.3f, 0.5f, 0.04, noise);

        addHit (out, sampleRate, start, 3000.0, 0.0f, 0.15f, 0.01, noise);
    }

    return out;
}

/** Exakt geschnittener Loop, dessen Einsaetze auf punktierten Achteln
    liegen — Kick und Snare halten das echte Raster, die Hi-Hats alle 0,75
    Schlaege verleiten die Autokorrelation zum 4/3-fachen Tempo.
*/
inline std::vector<float> dottedLoop (double bpm, int bars, double sampleRate)
{
    const double beatSamples = 60.0 / bpm * sampleRate;
    const auto length = static_cast<size_t> (std::llround (beatSamples * 4 * bars));
    std::vector<float> out (length, 0.0f);
    Noise noise;

    for (int beat = 0; beat < bars * 4; ++beat)
    {
        const auto start = static_cast<size_t> (std::llround (beat * beatSamples));

        if (beat % 4 == 0 || beat % 4 == 2)
            addHit (out, sampleRate, start, 55.0, 0.5f, 0.05f, 0.08, noise);
        else
            addHit (out, sampleRate, start, 200.0, 0.2f, 0.3f, 0.04, noise);
    }

    for (double t = 0.0; t < static_cast<double> (length); t += beatSamples * 0.75)
        addHit (out, sampleRate, static_cast<size_t> (t), 3000.0, 0.0f, 0.6f, 0.01, noise);

    return out;
}

inline AnalysisResult analyse (const std::vector<float>& audio, double sampleRate,
                               const AnalysisSettings& settings = {}, int blockSize = 4096)
{
    KeyAnalyzer analyzer;
    analyzer.prepare (sampleRate, settings);

    const int total = static_cast<int> (audio.size());
    for (int pos = 0; pos < total; pos += blockSize)
        analyzer.process (audio.data() + pos, std::min (blockSize, total - pos));

    return analyzer.finish();
}

} // namespace keyy::test
