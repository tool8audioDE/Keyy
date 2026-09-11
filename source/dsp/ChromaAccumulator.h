#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Fft.h"

namespace keyy
{

/** Sammelt ein stimmton-korrigiertes Chromagramm über eine ganze Datei.

    JE RAHMEN
    Hann-Fenster, Betragsspektrum, Spektralspitzen suchen, jede Spitze in
    eine Tonhöhe in Halbtönen umrechnen und in ein feines Tonklassen-
    Histogramm (10 Cent je Fach) eintragen. Nur Spitzen, nicht das ganze
    Spektrum: Das Rauschen von Hi-Hats und Snares verteilt sich breit und
    ragt kaum über seine Umgebung hinaus — Töne schon.

    Jeder Rahmen geht mit gleichem Gewicht ein, unabhängig von seiner
    Lautstärke. Sonst bestimmten die lautesten Stellen die Tonart allein,
    und bei Beats sind das die Einschläge der Kick. Stille Rahmen zählen
    gar nicht.

    STIMMTON IN EINEM DURCHGANG
    Die Abweichung jeder Spitze vom 440-Hz-Raster landet zusätzlich in einem
    Cent-Histogramm. Erst am Ende wird daraus der Stimmton bestimmt und das
    feine Histogramm damit auf 12 Tonklassen gefaltet. So braucht die Analyse
    keinen zweiten Durchgang, und derselbe Code taugt später für einen
    Listen-Modus, der blockweise zuhört.

    Der Stimmton ist der häufigste Wert, nicht der Mittelwert: Obertöne
    liegen nicht im gleichstufigen Raster — der fünfte 14 Cent, der siebte
    31 Cent zu tief. Ein Mittelwert würde davon mitgezogen, der häufigste
    Wert nicht.
*/
class ChromaAccumulator
{
public:
    struct Settings
    {
        /** 16384 Punkte: bei 11 kHz 0,67 Hz Auflösung. Um 50 Hz liegen
            Halbtöne nur 3 Hz auseinander, und genau dort spielt die 808.
        */
        int    fftOrder          = 14;
        int    hopSize           = 4096;
        double minHz             = 45.0;
        double maxHz             = 3500.0;

        /** Eine Spitze zählt nur, wenn sie so weit über dem Mittel ihrer
            Umgebung liegt. Hält das Rauschen von Drums heraus.

            An GiantSteps gemessen (301 Stücke, kreuzvalidiert): 1 → 25 %,
            1,5 → 45 %, 2 → 55 %, 3 → 54 %, 6 → 42 %, 10 → 30 % exakt. Zu
            streng verliert leise Akkordtöne, zu locker lässt Rauschen durch.
        */
        double peakThreshold     = 2.0;

        /** Gewicht einer Spitze = Betrag hoch x. Unter 1 wird der Abstand
            zwischen lauten Bässen und leiseren Akkordtönen kleiner.
            GiantSteps: 0 → 35 %, 0,25 → 52 %, 0,5 → 55 %, 1 → 49 %.
        */
        double magnitudeExponent = 0.5;

        /** Rahmen unter diesem Effektivwert (-60 dBFS) zählen nicht. */
        double silenceRms        = 1.0e-3;

        /** Gewicht eines Rahmens = (Anteil der Spitzen am Spektrum) hoch x.
            0 = jeder Rahmen gleich. Größer als 0 drückt Rahmen, in denen
            Drums und Rauschen überwiegen. GiantSteps: 0 → 53 %, 1 → 55 %,
            2 → 54 %.
        */
        double tonalityWeighting = 1.0;
    };

    static constexpr int binsPerSemitone = 10;
    static constexpr int fineBins = 12 * binsPerSemitone;
    static constexpr int centBins = 100;

    void prepare (double sampleRate, const Settings& settings);
    void reset();

    void process (const float* samples, int numSamples);

    /** Am Ende einer Datei: Ein Sample, das kürzer als ein Rahmen ist, wird
        aufgefüllt und einmal analysiert, damit es überhaupt ein Ergebnis gibt.
    */
    void flush();

    int getNumFrames() const noexcept { return numFrames; }

    /** Abweichung vom 440-Hz-Raster in Cent, -50..+50. */
    double getTuningCents() const;

    /** Stimmton-korrigiert, 0 = C, Summe 1. */
    std::array<double, 12> getChroma() const;

    const std::array<double, fineBins>& getFineChroma() const noexcept { return fine; }

private:
    void analyseFrame (const float* samples);

    struct Peak
    {
        double pitch;
        double weight;
    };

    Settings settings;
    double sampleRate = 11025.0;
    std::unique_ptr<Fft> fft;
    std::vector<float> window, frame, magnitudes, pending;
    std::vector<double> prefix;
    std::vector<Peak> peaks;
    std::array<double, fineBins> fine {};
    std::array<double, centBins> cents {};
    int numFrames = 0;
};

} // namespace keyy
