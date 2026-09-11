#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include "TestHelpers.h"

using namespace keyy;

TEST_CASE ("Alle 24 Tonarten werden an einer Kadenz erkannt, mit jedem Profil")
{
    const double sampleRate = 22050.0;

    AnalysisSettings settings;
    settings.detectTempo = false;

    for (int index = 0; index < 24; ++index)
    {
        const Key key = Key::fromIndex (index);
        const auto result = test::analyse (test::cadence (key, sampleRate), sampleRate, settings);
        REQUIRE (result.valid);

        for (const auto profile : allProfiles)
        {
            const auto estimate = estimateKey (result.chroma, profile);
            INFO (getProfile (profile).name << ": " << key.name() << " erkannt als " << estimate.key.name());
            CHECK (estimate.key == key);
        }
    }
}

TEST_CASE ("Verstimmung um fast einen Viertelton wird ausgeglichen")
{
    // A = 450 Hz liegt 39 Cent ueber dem Raster. Ohne Stimmton-Korrektur
    // fielen die Toene fast mittig zwischen zwei Tonklassen.
    const double sampleRate = 22050.0;
    const Key fMinor { 5, Mode::Minor };

    AnalysisSettings settings;
    settings.detectTempo = false;
    const auto result = test::analyse (test::cadence (fMinor, sampleRate, 1.0, 450.0), sampleRate, settings);

    REQUIRE (result.valid);
    CHECK (result.key == fMinor);
    CHECK (std::abs (result.tuningCents - 1200.0 * std::log2 (450.0 / 440.0)) < 3.0);
}

TEST_CASE ("Kadenz ueber einem Drum-Loop wird trotzdem erkannt")
{
    const double sampleRate = 44100.0;
    const Key key { 3, Mode::Minor };    // Eb-Moll

    auto audio = test::cadence (key, sampleRate, 60.0 / 140.0 * 4.0, 440.0, 2);    // ein Akkord je Takt
    const auto drums = test::drumLoop (140.0, 8, sampleRate);

    audio.resize (std::max (audio.size(), drums.size()), 0.0f);
    for (size_t i = 0; i < drums.size(); ++i)
        audio[i] += drums[i];

    const auto result = test::analyse (audio, sampleRate);

    REQUIRE (result.valid);
    CHECK (result.key == key);
}

TEST_CASE ("Ergebnis haengt nicht von der Blockgroesse ab")
{
    const double sampleRate = 44100.0;
    auto audio = test::cadence (Key { 7, Mode::Major }, sampleRate, 1.0);
    const auto drums = test::drumLoop (120.0, 4, sampleRate);
    for (size_t i = 0; i < std::min (audio.size(), drums.size()); ++i)
        audio[i] += drums[i];

    const auto small = test::analyse (audio, sampleRate, {}, 64);
    const auto large = test::analyse (audio, sampleRate, {}, 10000);

    REQUIRE (small.valid);
    REQUIRE (large.valid);
    CHECK (small.key == large.key);
    CHECK (small.tuningCents == large.tuningCents);
    CHECK (small.bpm == large.bpm);

    for (size_t i = 0; i < 12; ++i)
        CHECK (std::abs (small.chroma[i] - large.chroma[i]) < 1.0e-12);
}
