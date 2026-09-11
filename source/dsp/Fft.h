#pragma once

#include <complex>
#include <vector>

namespace keyy
{

/** Radix-2-FFT.

    Bewusst selbst geschrieben statt eingebunden: Der DSP-Kern soll ohne
    Abhängigkeiten bauen, und die Analyse braucht nur Betragsspektren weniger
    fester Größen. Ein ganzer Song kostet einige tausend Transformationen —
    dafür ist diese schlichte Fassung um Größenordnungen schnell genug.
*/
class Fft
{
public:
    /** @param order  Größe als Zweierpotenz: 2^order Punkte */
    explicit Fft (int order);

    int getSize() const noexcept { return size; }

    /** Betragsspektrum eines reellen Signals.

        @param input          getSize() Werte
        @param magnitudesOut  getSize() / 2 + 1 Werte
    */
    void magnitudes (const float* input, float* magnitudesOut);

    /** Vorwärtstransformation an Ort und Stelle. */
    void perform (std::complex<float>* data) const noexcept;

private:
    int size;
    std::vector<std::complex<float>> twiddles;
    std::vector<int> bitReversed;
    std::vector<std::complex<float>> work;
};

} // namespace keyy
