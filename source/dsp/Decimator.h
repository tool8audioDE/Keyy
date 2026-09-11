#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "DspCommon.h"

namespace keyy
{

/** Senkt die Abtastrate um einen ganzzahligen Faktor auf etwa 11 kHz.

    Tonart und Tempo stecken unterhalb von 5 kHz. Bei einem Viertel der
    Abtastrate braucht dieselbe Frequenzauflösung ein Viertel so große
    FFTs — und die Analyse eines Songs dauert Sekundenbruchteile statt
    Sekunden.

    Ganzzahlig statt auf exakt 11025 Hz: Ohne Interpolation ist das
    Ergebnis frei von deren Fehlern. 48 kHz landen so bei 12 kHz, 44,1 kHz
    bei 11025 Hz — die Analyse rechnet ohnehin mit der tatsächlichen Rate.

    Davor ein Butterworth-Tiefpass achter Ordnung, damit nichts von oberhalb
    der neuen Nyquist-Frequenz als falscher Ton zurückfällt.
*/
class Decimator
{
public:
    void prepare (double inputRate, double targetRate)
    {
        factor = std::max (1, static_cast<int> (std::lround (inputRate / targetRate)));
        outputRate = inputRate / factor;

        // Güten der vier Teilfilter eines Butterworth-Tiefpasses 8. Ordnung.
        static constexpr double q[4] { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };

        for (size_t i = 0; i < filters.size(); ++i)
            filters[i].setLowPass (inputRate, 0.42 * outputRate, q[i]);

        reset();
    }

    void reset() noexcept
    {
        for (auto& filter : filters)
            filter.reset();

        phase = 0;
    }

    double getOutputRate() const noexcept { return outputRate; }
    int getFactor() const noexcept { return factor; }

    /** Hängt die dezimierten Samples an output an. Der Zustand bleibt über
        Blockgrenzen erhalten — das Ergebnis hängt nicht von der Blockgröße ab.
    */
    void process (const float* input, int numSamples, std::vector<float>& output)
    {
        if (factor == 1)
        {
            output.insert (output.end(), input, input + numSamples);
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            double x = input[i];
            for (auto& filter : filters)
                x = filter.process (x);

            if (phase == 0)
                output.push_back (static_cast<float> (x));

            if (++phase >= factor)
                phase = 0;
        }
    }

private:
    std::array<Biquad, 4> filters;
    int factor = 1;
    int phase = 0;
    double outputRate = 44100.0;
};

} // namespace keyy
