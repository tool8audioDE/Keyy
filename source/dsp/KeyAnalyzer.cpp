#include "KeyAnalyzer.h"

#include <algorithm>
#include <cmath>

#include "DspCommon.h"

namespace keyy
{

namespace
{
    /** Erster Taktanfang in Sekunden.

        Gesucht wird die Verschiebung, bei der das Taktraster die meiste
        Einsatzenergie trifft. Bei Loops liegt das Ergebnis fast immer bei
        0 — sie sind auf den Takt geschnitten —, aber Beats mit Auftakt
        haben es nicht, und Raten wäre dort schlechter als Messen.

        Das Fenster von ±40 ms fängt auf, dass der Schlag nicht exakt auf
        dem Raster sitzt.
    */
    double findBarPhase (const std::vector<float>& envelope, double envelopeRate, double barSeconds)
    {
        const int count = static_cast<int> (envelope.size());
        const int barSamples = static_cast<int> (std::round (barSeconds * envelopeRate));

        if (count <= 0 || barSamples <= 1)
            return 0.0;

        const int slack = std::max (1, static_cast<int> (std::round (0.04 * envelopeRate)));

        double bestPhase = 0.0, bestScore = -1.0;

        for (int offset = 0; offset < barSamples; ++offset)
        {
            double score = 0.0;
            for (int position = offset; position < count; position += barSamples)
            {
                float peak = 0.0f;
                for (int d = -slack; d <= slack; ++d)
                {
                    const int k = position + d;
                    if (k >= 0 && k < count)
                        peak = std::max (peak, envelope[static_cast<size_t> (k)]);
                }
                score += peak;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestPhase = offset / envelopeRate;
            }
        }

        return bestPhase;
    }
}

void KeyAnalyzer::prepare (double sampleRate, const AnalysisSettings& newSettings)
{
    settings = newSettings;
    inputRate = sampleRate;
    totalSamples = 0;

    decimator.prepare (sampleRate, settings.analysisRate);

    auto chromaSettings = settings.chroma;
    chromaSettings.recordFrames = settings.chroma.recordFrames || settings.barWeighting > 0.0;
    chroma.prepare (decimator.getOutputRate(), chromaSettings);
    tempo.prepare (decimator.getOutputRate(), settings.tempo);

    decimated.clear();
}

void KeyAnalyzer::process (const float* mono, int numSamples)
{
    if (numSamples <= 0)
        return;

    decimated.clear();
    decimator.process (mono, numSamples, decimated);

    if (! decimated.empty())
    {
        const int count = static_cast<int> (decimated.size());
        chroma.process (decimated.data(), count);

        if (settings.detectTempo)
            tempo.process (decimated.data(), count);
    }

    totalSamples += numSamples;
}

double KeyAnalyzer::getSecondsProcessed() const noexcept
{
    return inputRate > 0.0 ? static_cast<double> (totalSamples) / inputRate : 0.0;
}

AnalysisResult KeyAnalyzer::finish()
{
    chroma.flush();

    AnalysisResult result;
    result.durationSeconds = getSecondsProcessed();
    result.analysedFrames = chroma.getNumFrames();

    // Das Tempo muss vor das Chromagramm: Ohne Takt gibt es keine
    // Gewichtung des Taktanfangs.
    if (settings.detectTempo)
    {
        const auto t = tempo.estimate (result.durationSeconds);
        result.tempoValid        = t.valid;
        result.bpm               = t.bpm;
        result.onsetBpm          = t.onsetBpm;
        result.bpmFromLoopLength = t.fromLoopLength;
        result.tempoSalience     = t.salience;
    }

    if (result.analysedFrames > 0)
    {
        result.chroma = chroma.getChroma();

        const int recorded = chroma.getNumRecordedFrames();

        if (settings.barWeighting > 0.0 && result.tempoValid && result.bpm > 0.0 && recorded > 0)
        {
            result.barSeconds = 240.0 / result.bpm;     // vier Viertel je Takt

            // Kam das Tempo aus der Looplaenge, ist die Datei auf ganze
            // Takte geschnitten und der erste Taktanfang liegt bei 0.
            // Das ist sicherer als jede Messung: An Loops sucht sich
            // findBarPhase sonst die lauteste Stelle, und das ist oft die
            // Snare auf der Zwei statt der Kick auf der Eins.
            result.barPhase = result.bpmFromLoopLength
                                ? 0.0
                                : findBarPhase (tempo.getOnsetEnvelope(), tempo.getEnvelopeRate(), result.barSeconds);

            std::vector<double> weights (static_cast<size_t> (recorded));
            for (int i = 0; i < recorded; ++i)
            {
                const double offset = chroma.getRecordedFrameCentre (i) - result.barPhase;
                const double fraction = wrap (offset / result.barSeconds, 1.0);

                // 1 auf der Eins, 0 in der Taktmitte, weich dazwischen.
                const double bump = 0.5 * (1.0 + std::cos (2.0 * pi * fraction));
                weights[static_cast<size_t> (i)] = 1.0 + settings.barWeighting * bump;
            }

            result.chroma = chroma.getChroma (weights);
            result.barWeighted = true;
        }

        result.tuningCents = chroma.getTuningCents();
        result.referenceHz = 440.0 * std::pow (2.0, result.tuningCents / 1200.0);

        const auto estimate = estimateKey (result.chroma, settings.profile);
        result.key           = estimate.key;
        result.keyScore      = estimate.score;
        result.runnerUp      = estimate.runnerUp;
        result.runnerUpScore = estimate.runnerUpScore;
        result.keyScores     = estimate.scores;
        result.valid = true;
    }

    if (settings.chroma.recordFrames && chroma.getNumRecordedFrames() > 0)
    {
        result.frames = chroma.getRecordedFine();
        result.frameBins = ChromaAccumulator::fineBins;
    }

    return result;
}

} // namespace keyy
