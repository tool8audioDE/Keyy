#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "TestHelpers.h"

using namespace keyy;

TEST_CASE ("Tempo eines Klicks wird auf ein halbes BPM genau erkannt")
{
    for (const double bpm : { 85.0, 128.0, 140.0 })
    {
        // 20,3 s: bewusst keine ganze Zahl von Takten, der Loop-Abgleich
        // darf hier nicht greifen.
        const auto result = test::analyse (test::clickTrack (bpm, 44100.0, 20.3), 44100.0);

        INFO (bpm << " BPM erkannt als " << result.bpm);
        REQUIRE (result.tempoValid);
        CHECK (std::abs (result.bpm - bpm) < 0.5);
        CHECK_FALSE (result.bpmFromLoopLength);
    }
}

TEST_CASE ("Weit ausserhalb der Mitte darf das halbe Tempo herauskommen")
{
    // Ein gleichmaessiger Klick auf 174 BPM ist von 87 BPM mit Offbeats
    // nicht zu unterscheiden. Die Vorliebe fuer die Mitte um 110 BPM waehlt
    // 87 — dafuer gibt es die Taste x2. Genau muss es trotzdem sein.
    const auto result = test::analyse (test::clickTrack (174.0, 44100.0, 20.3), 44100.0);

    INFO ("erkannt: " << result.bpm);
    REQUIRE (result.tempoValid);
    CHECK ((std::abs (result.bpm - 174.0) < 0.5 || std::abs (result.bpm - 87.0) < 0.25));
}

TEST_CASE ("Exakt geschnittener Loop: Tempo aus der Laenge, ganzzahlig")
{
    // Vier Takte 93 BPM, auf das Sample genau. Genau so liegen Loops in
    // Sample-Packs vor.
    const auto result = test::analyse (test::drumLoop (93.0, 4, 44100.0), 44100.0);

    REQUIRE (result.tempoValid);
    CHECK (result.bpm == 93.0);
    CHECK (result.bpmFromLoopLength);
}

TEST_CASE ("Loop mit Stille am Ende: kein Loop-Abgleich, Einsaetze entscheiden")
{
    const auto result = test::analyse (test::drumLoop (93.0, 4, 44100.0, 0.37), 44100.0);

    INFO ("erkannt: " << result.bpm);
    REQUIRE (result.tempoValid);
    CHECK_FALSE (result.bpmFromLoopLength);
    CHECK (std::abs (result.bpm - 93.0) < 1.0);
}

TEST_CASE ("Diagnose: Spitzheit der Einsaetze", "[.diagnose]")
{
    const double sr = 44100.0;

    std::vector<float> pad (static_cast<size_t> (8.0 * sr), 0.0f);
    for (const double hz : { 220.0, 277.18, 329.63 })
        test::addTone (pad, sr, hz, 0, pad.size(), 0.2f);

    auto cadenceDrums = test::cadence (Key { 3, Mode::Minor }, sr, 60.0 / 140.0 * 4.0, 440.0, 2);
    const auto drums = test::drumLoop (140.0, 8, sr);
    cadenceDrums.resize (std::max (cadenceDrums.size(), drums.size()), 0.0f);
    for (size_t i = 0; i < drums.size(); ++i)
        cadenceDrums[i] += drums[i];

    struct Fall { const char* name; std::vector<float> audio; };
    const Fall faelle[] {
        { "pad", pad },
        { "kadenz", test::cadence (Key { 0, Mode::Major }, sr, 60.0 / 90.0 * 4.0, 440.0, 3) },
        { "kadenz+drums", cadenceDrums },
        { "drumloop93", test::drumLoop (93.0, 4, sr) },
        { "klick128", test::clickTrack (128.0, sr, 20.3) },
    };

    for (const auto& fall : faelle)
    {
        const auto r = test::analyse (fall.audio, sr);
        WARN (fall.name << ": bpm " << r.bpm << " gueltig " << r.tempoValid << " spitzheit " << r.tempoSalience);
    }
}

TEST_CASE ("Ein gehaltener Akkord hat kein Tempo")
{
    const double sampleRate = 22050.0;
    std::vector<float> pad (static_cast<size_t> (8.0 * sampleRate), 0.0f);
    for (const double hz : { 220.0, 277.18, 329.63 })
        test::addTone (pad, sampleRate, hz, 0, pad.size(), 0.2f);

    const auto result = test::analyse (pad, sampleRate);

    CHECK (result.valid);
    CHECK_FALSE (result.tempoValid);
}
