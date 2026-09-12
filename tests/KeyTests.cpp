#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "dsp/Key.h"

using namespace keyy;

TEST_CASE ("Tonartnamen in englischer Schreibweise, schwarze Tasten mit b ausser F#")
{
    CHECK (Key { 5,  Mode::Minor }.name() == "F Minor");
    CHECK (Key { 8,  Mode::Major }.name() == "Ab Major");
    CHECK (Key { 6,  Mode::Minor }.shortName() == "F#m");
    CHECK (Key { 10, Mode::Major }.shortName() == "Bb");
}

TEST_CASE ("Paralleltonart und Index")
{
    CHECK (Key { 5, Mode::Minor }.relative() == Key { 8, Mode::Major });   // f-Moll <-> As-Dur
    CHECK (Key { 0, Mode::Major }.relative() == Key { 9, Mode::Minor });   // C-Dur <-> a-Moll

    for (int index = 0; index < 24; ++index)
    {
        const Key key = Key::fromIndex (index);
        CHECK (key.index() == index);
        CHECK (key.relative().relative() == key);
    }
}

TEST_CASE ("MIREX-Wertung")
{
    const Key fMinor { 5, Mode::Minor };

    CHECK (mirexScore (fMinor, fMinor) == 1.0);
    CHECK (mirexScore (Key { 0,  Mode::Minor }, fMinor) == 0.5);   // c-Moll: Quinte darueber
    CHECK (mirexScore (Key { 10, Mode::Minor }, fMinor) == 0.5);   // b-Moll: Quinte darunter
    CHECK (mirexScore (Key { 8,  Mode::Major }, fMinor) == 0.3);   // As-Dur: Paralleltonart
    CHECK (mirexScore (Key { 5,  Mode::Major }, fMinor) == 0.2);   // F-Dur: gleichnamig
    CHECK (mirexScore (Key { 6,  Mode::Minor }, fMinor) == 0.0);
    CHECK (mirexScore (Key { 0,  Mode::Major }, fMinor) == 0.0);   // Quinte, aber anderes Tongeschlecht
}

TEST_CASE ("Tonart aus Dateinamen")
{
    struct Fall
    {
        const char* name;
        std::optional<Key> expected;
    };

    const Fall faelle[] {
        { "Loop_Fm_140bpm.wav",          Key { 5,  Mode::Minor } },
        { "Dark Keys F#min 75 BPM.wav",  Key { 6,  Mode::Minor } },
        { "Gb minor - pad.wav",          Key { 6,  Mode::Minor } },
        { "melody_Bbmaj_120.wav",        Key { 10, Mode::Major } },
        { "C_Major_Chords.wav",          Key { 0,  Mode::Major } },
        { "Am_Guitar.wav",               Key { 9,  Mode::Minor } },
        { "Ebm_808.wav",                 Key { 3,  Mode::Minor } },
        { "Csharp minor.wav",            Key { 1,  Mode::Minor } },
        { "Eb minor",                    Key { 3,  Mode::Minor } },   // GiantSteps-Beschriftung

        { "149_5.wav",                   std::nullopt },
        { "Drum_Loop_140bpm.wav",        std::nullopt },
        { "Emaj7_stab.wav",              std::nullopt },   // Akkord, keine Tonart
        { "A_Loop.wav",                  std::nullopt },   // Ton ohne Tongeschlecht
        { "Chord_Loop.wav",              std::nullopt },
        { "Lead_FM_Synth.wav",           std::nullopt },   // FM-Synthese, nicht F-Dur
        { "Dark_Bass_Ebony.wav",         std::nullopt },
    };

    for (const auto& fall : faelle)
    {
        INFO (fall.name);
        const auto key = parseKey (fall.name);

        REQUIRE (key.has_value() == fall.expected.has_value());
        if (key)
            CHECK (key->name() == fall.expected->name());
    }
}

TEST_CASE ("Tempo aus Dateinamen")
{
    CHECK (parseBpm ("Loop_Fm_140bpm.wav") == 140.0);
    CHECK (parseBpm ("Dark Keys F#min 75 BPM.wav") == 75.0);
    CHECK (parseBpm ("Beat 92BPM.mp3") == 92.0);
    CHECK_FALSE (parseBpm ("149_5.wav").has_value());
    CHECK (parseBpm ("Ghosthack Bass Loop_120_Decay Bass_E Minor.wav") == 120.0);
    CHECK (parseBpm ("Pad Tempo 90 Am.wav") == 90.0);
    CHECK_FALSE (parseBpm ("Loop_4_Bell Synth.wav").has_value());
    CHECK_FALSE (parseBpm ("Loop_1000bpm.wav").has_value());
}
