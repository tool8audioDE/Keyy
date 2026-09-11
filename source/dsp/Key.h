#pragma once

#include <optional>
#include <string>

namespace keyy
{

enum class Mode { Major, Minor };

/** Eine Tonart: Grundton als Tonklasse (0 = C) und Tongeschlecht.

    Enharmonische Schreibweisen fallen hier zusammen — Gb-Moll und F#-Moll
    sind dieselbe Tonart. Die Schreibweise entsteht erst bei der Ausgabe.
*/
struct Key
{
    int  tonic = 0;
    Mode mode  = Mode::Major;

    /** 0..11 Dur (C..B), 12..23 Moll (C..B). */
    int index() const noexcept;
    static Key fromIndex (int index) noexcept;

    /** Paralleltonart (englisch "relative key"): C-Dur <-> a-Moll.

        Achtung, Begriffsfalle: Die englische "parallel key" ist die
        gleichnamige Tonart (C-Dur <-> c-Moll), nicht die Paralleltonart.
    */
    Key relative() const noexcept;

    /** "F Minor", "Ab Major" — die Schreibweise von Auto-Key und FL Studio. */
    std::string name() const;

    /** "Fm", "Ab" — die Schreibweise in Dateinamen von Sample-Packs. */
    std::string shortName() const;

    bool operator== (const Key& other) const noexcept { return tonic == other.tonic && mode == other.mode; }
    bool operator!= (const Key& other) const noexcept { return ! (*this == other); }
};

/** Tonname mit b für die schwarzen Tasten, außer F#: Db Eb F# Ab Bb. */
const char* noteName (int pitchClass) noexcept;

/** Verhältnis einer erkannten Tonart zur richtigen, wie im MIREX-Wettbewerb. */
enum class KeyRelation
{
    Same,       ///< exakt
    Fifth,      ///< Quinte darüber oder darunter, gleiches Tongeschlecht
    Relative,   ///< Paralleltonart
    Parallel,   ///< gleichnamige Tonart
    Other
};

KeyRelation relationOf (const Key& estimated, const Key& reference) noexcept;

/** MIREX-Wertung: exakt 1, Quinte 0,5, Paralleltonart 0,3, gleichnamig 0,2. */
double mirexScore (const Key& estimated, const Key& reference) noexcept;

/** "exakt", "Quinte", "Parallele", "Gleichnamig", "andere" */
const char* relationName (KeyRelation relation) noexcept;

/** Liest eine Tonart aus einem Dateinamen oder einer Beschriftung.

    Erkannt werden "Fm", "F#min", "Gbminor", "Bbmaj", "Csharp minor",
    "C_Major" und ähnliches. Ein Ton ohne Tongeschlecht ("A_Loop") zählt
    nicht — dort ist nicht zu unterscheiden, ob Dur gemeint ist oder ob der
    Buchstabe gar keine Tonart bezeichnet. Stehen mehrere Tonarten im Text,
    gilt die letzte: Die steht in Sample-Namen üblicherweise hinten.
*/
std::optional<Key> parseKey (const std::string& text);

/** Liest ein Tempo aus einem Dateinamen: "140bpm", "75 BPM". */
std::optional<double> parseBpm (const std::string& text);

} // namespace keyy
