#pragma once

#include <cmath>

namespace keyy
{

constexpr double pi = 3.14159265358979323846;

/** Tonhöhe in Halbtönen (MIDI-Notennummer, gebrochen), bezogen auf A = 440 Hz.

    Die gesamte Analyse rechnet in Halbtönen statt in Hertz: Tonklassen,
    Stimmton und Intervalle sind in Halbtönen linear, in Hertz nicht.
*/
inline double midiFromHz (double hz, double referenceA = 440.0) noexcept
{
    return 69.0 + 12.0 * std::log2 (hz / referenceA);
}

inline double hzFromMidi (double midi, double referenceA = 440.0) noexcept
{
    return referenceA * std::pow (2.0, (midi - 69.0) / 12.0);
}

/** Rest im Bereich [0, m) — auch für negative x, anders als std::fmod. */
inline double wrap (double x, double m) noexcept
{
    const double r = std::fmod (x, m);
    return r < 0.0 ? r + m : r;
}

inline int wrap (int x, int m) noexcept
{
    const int r = x % m;
    return r < 0 ? r + m : r;
}

/** Biquad in Direktform II transponiert, in doppelter Genauigkeit.

    Doppelte Genauigkeit, weil der Dezimierer einen Tiefpass bei einem
    Zehntel der Abtastrate braucht. Dort liegen die Pole nahe am
    Einheitskreis, und in float wird so ein Filter rauschig.
*/
struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    void reset() noexcept { z1 = z2 = 0.0; }

    inline double process (double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    /** Tiefpass nach Robert Bristow-Johnson. */
    void setLowPass (double sampleRate, double cutoffHz, double q) noexcept
    {
        const double w0    = 2.0 * pi * cutoffHz / sampleRate;
        const double cosw0 = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);

        const double a0 = 1.0 + alpha;
        b0 = (1.0 - cosw0) * 0.5 / a0;
        b1 = (1.0 - cosw0) / a0;
        b2 = (1.0 - cosw0) * 0.5 / a0;
        a1 = (-2.0 * cosw0) / a0;
        a2 = (1.0 - alpha) / a0;
    }
};

} // namespace keyy
