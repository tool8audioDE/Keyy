#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "TestHelpers.h"
#include "dsp/ChromaAccumulator.h"
#include "dsp/Fft.h"

using namespace keyy;

TEST_CASE ("FFT: ein Sinus auf einem Fach landet genau dort")
{
    Fft fft (10);
    const int n = fft.getSize();

    std::vector<float> input (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        input[static_cast<size_t> (i)] = static_cast<float> (std::sin (2.0 * pi * 37.0 * i / n));

    std::vector<float> magnitudes (static_cast<size_t> (n / 2 + 1));
    fft.magnitudes (input.data(), magnitudes.data());

    const auto peak = std::max_element (magnitudes.begin(), magnitudes.end()) - magnitudes.begin();
    CHECK (peak == 37);
    CHECK (magnitudes[37] == Catch::Approx (n / 2.0).epsilon (0.001));
}

TEST_CASE ("Chroma: ein Ton A landet auf der Tonklasse A")
{
    const double sampleRate = 11025.0;
    std::vector<float> audio (static_cast<size_t> (3.0 * sampleRate), 0.0f);
    test::addTone (audio, sampleRate, 220.0, 0, audio.size(), 0.5f, 1);

    ChromaAccumulator chroma;
    chroma.prepare (sampleRate, {});
    chroma.process (audio.data(), static_cast<int> (audio.size()));
    chroma.flush();

    const auto c = chroma.getChroma();
    CHECK (std::max_element (c.begin(), c.end()) - c.begin() == 9);
    CHECK (c[9] > 0.9);
    CHECK (std::abs (chroma.getTuningCents()) < 2.0);
}

TEST_CASE ("Stimmton wird auf wenige Cent genau erkannt, trotz Obertoenen")
{
    // 432 Hz: "Verschwoerungsstimmung", 437 Hz: wie im Auto-Key-Beispiel,
    // 446 Hz: alte Platte, etwas zu schnell abgespielt.
    for (const double referenceA : { 432.0, 437.0, 446.0 })
    {
        const double sampleRate = 22050.0;
        const auto audio = test::cadence (Key { 0, Mode::Major }, sampleRate, 1.0, referenceA);

        AnalysisSettings settings;
        settings.detectTempo = false;
        const auto result = test::analyse (audio, sampleRate, settings);

        const double expected = 1200.0 * std::log2 (referenceA / 440.0);
        INFO ("A = " << referenceA << " Hz, erwartet " << expected << " Cent, gemessen " << result.tuningCents);

        REQUIRE (result.valid);
        CHECK (std::abs (result.tuningCents - expected) < 3.0);
        CHECK (result.referenceHz == Catch::Approx (referenceA).margin (0.8));
    }
}

TEST_CASE ("Stille liefert kein Ergebnis")
{
    const std::vector<float> silence (44100 * 3, 0.0f);
    const auto result = test::analyse (silence, 44100.0);

    CHECK_FALSE (result.valid);
    CHECK_FALSE (result.tempoValid);
}
