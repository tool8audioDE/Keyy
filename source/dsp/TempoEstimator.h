#pragma once

#include <memory>
#include <vector>

#include "Fft.h"

namespace keyy
{

/** Tempo aus Einsätzen, bei Loops abgeglichen mit der Dateilänge.

    EINSÄTZE
    Spectral Flux: je Rahmen (46 ms, alle 6 ms) die Summe aller Zunahmen
    des logarithmierten Spektrums gegenüber dem Rahmen davor. Ein Einsatz —
    Kick, Snare, Akkordwechsel — ist ein plötzlicher Zuwachs an Energie in
    vielen Frequenzen zugleich.

    TEMPO
    Autokorrelation der Einsatzkurve, ausgewertet als Kamm: Für jedes
    Kandidatentempo zählen die Werte beim Schlagabstand und seinen
    Vielfachen. Ein einzelner Wert wäre anfällig für Synkopen; der Kamm
    verlangt, dass sich das Muster über mehrere Schläge wiederholt.

    Halbe und doppelte Tempi sind aus den Einsätzen allein oft nicht zu
    unterscheiden (70 oder 140?). Eine schwache Vorliebe für die Mitte um
    110 BPM entscheidet; die Tasten /2 und x2 in der Oberfläche korrigieren
    den Rest.

    LOOPLÄNGE
    Loops aus Sample-Packs sind fast immer eine ganze Zahl von Takten lang,
    exakt auf das Sample geschnitten. Ergibt die Dateilänge bei 1, 2, 4, 8 …
    Takten ein ganzzahliges Tempo nahe der Schätzung, ist das genauer als
    jede Einsatzmessung — dann gilt dieser Wert.
*/
class TempoEstimator
{
public:
    struct Settings
    {
        int    fftOrder       = 9;       ///< 512 Punkte, bei 11 kHz 46 ms
        int    hopSize        = 64;      ///< bei 11 kHz knapp 6 ms
        double minBpm         = 60.0;
        double maxBpm         = 200.0;
        double preferredBpm   = 110.0;   ///< Mitte der schwachen Vorliebe
        double maxLoopSeconds = 64.0;    ///< längere Dateien gelten nicht als Loop

        /** Mindest-Spitzheit der Einsätze (siehe Result::salience). An
            synthetischen Signalen gemessen: gehaltener Akkord 5,6, Akkord-
            wechsel je Takt 12, Drum-Loop 17, Klick 20. An echten Beats
            noch zu überprüfen.
        */
        double minSalience    = 7.5;
    };

    struct Result
    {
        bool   valid = false;
        double bpm = 0.0;
        double onsetBpm = 0.0;           ///< aus den Einsätzen allein
        bool   fromLoopLength = false;

        /** Wie spitz die Einsätze sind: Mittel der stärksten 5 Prozent
            geteilt durch das Gesamtmittel. Schläge sind kurz und hoch,
            Schwebungen zwischen gehaltenen Tönen weich und breit.
        */
        double salience = 0.0;
    };

    void prepare (double sampleRate, const Settings& settings);
    void reset();

    void process (const float* samples, int numSamples);

    /** @param durationSeconds  exakte Länge der Datei, für den Loop-Abgleich */
    Result estimate (double durationSeconds) const;

    const std::vector<float>& getOnsetEnvelope() const noexcept { return envelope; }
    double getEnvelopeRate() const noexcept { return sampleRate / settings.hopSize; }

private:
    void analyseFrame (const float* samples);

    Settings settings;
    double sampleRate = 11025.0;
    std::unique_ptr<Fft> fft;
    std::vector<float> window, frame, magnitudes, previous, pending;
    std::vector<float> envelope;
    bool hasPrevious = false;
};

} // namespace keyy
