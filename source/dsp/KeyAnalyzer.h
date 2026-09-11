#pragma once

#include <array>
#include <vector>

#include "ChromaAccumulator.h"
#include "Decimator.h"
#include "Key.h"
#include "KeyEstimator.h"
#include "KeyProfiles.h"
#include "TempoEstimator.h"

namespace keyy
{

struct AnalysisSettings
{
    Profile profile = defaultProfile;
    ChromaAccumulator::Settings chroma;
    TempoEstimator::Settings tempo;
    bool detectTempo = true;
    double analysisRate = 11025.0;
};

struct AnalysisResult
{
    /** false: nichts Tonales gefunden (Stille, zu kurz). */
    bool valid = false;

    Key    key;
    double keyScore = 0.0;
    Key    runnerUp;
    double runnerUpScore = 0.0;
    std::array<double, 24> keyScores {};
    std::array<double, 12> chroma {};

    double tuningCents = 0.0;
    double referenceHz = 440.0;

    bool   tempoValid = false;
    double bpm = 0.0;
    double onsetBpm = 0.0;
    bool   bpmFromLoopLength = false;
    double tempoSalience = 0.0;

    double durationSeconds = 0.0;
    int    analysedFrames = 0;
};

/** Die ganze Analyse einer Datei hinter einer Schnittstelle.

    Blockweise gefüttert, am Ende einmal ausgewertet. Die Blockgröße spielt
    keine Rolle für das Ergebnis. Genau dadurch kann dieselbe Klasse die
    Datei im Plugin, im CLI und in den Tests analysieren — und später im
    Listen-Modus das laufende Signal der DAW.
*/
class KeyAnalyzer
{
public:
    void prepare (double sampleRate, const AnalysisSettings& settings = {});

    /** Mono-Samples in der ursprünglichen Abtastrate. */
    void process (const float* mono, int numSamples);

    AnalysisResult finish();

    double getSecondsProcessed() const noexcept;

private:
    AnalysisSettings settings;
    double inputRate = 44100.0;
    long long totalSamples = 0;

    Decimator decimator;
    ChromaAccumulator chroma;
    TempoEstimator tempo;
    std::vector<float> decimated;
};

} // namespace keyy
