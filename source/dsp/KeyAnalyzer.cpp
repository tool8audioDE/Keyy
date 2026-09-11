#include "KeyAnalyzer.h"

#include <cmath>

namespace keyy
{

void KeyAnalyzer::prepare (double sampleRate, const AnalysisSettings& newSettings)
{
    settings = newSettings;
    inputRate = sampleRate;
    totalSamples = 0;

    decimator.prepare (sampleRate, settings.analysisRate);
    chroma.prepare (decimator.getOutputRate(), settings.chroma);
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

    if (result.analysedFrames > 0)
    {
        result.chroma = chroma.getChroma();
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

    if (settings.detectTempo)
    {
        const auto t = tempo.estimate (result.durationSeconds);
        result.tempoValid        = t.valid;
        result.bpm               = t.bpm;
        result.onsetBpm          = t.onsetBpm;
        result.bpmFromLoopLength = t.fromLoopLength;
        result.tempoSalience     = t.salience;
    }

    return result;
}

} // namespace keyy
