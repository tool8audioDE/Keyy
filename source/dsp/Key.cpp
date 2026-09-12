#include "Key.h"

#include <cctype>
#include <vector>

#include "DspCommon.h"

namespace keyy
{

namespace
{
    const char* const noteNames[12] { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

    std::string toLower (std::string text)
    {
        for (auto& c : text)
            c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));

        return text;
    }

    /** Zerlegt an allem, was weder Buchstabe noch Ziffer noch '#' ist. */
    std::vector<std::string> tokenize (const std::string& text)
    {
        std::vector<std::string> tokens;
        std::string current;

        for (const char c : text)
        {
            if (std::isalnum (static_cast<unsigned char> (c)) || c == '#')
            {
                current += c;
            }
            else if (! current.empty())
            {
                tokens.push_back (current);
                current.clear();
            }
        }

        if (! current.empty())
            tokens.push_back (current);

        return tokens;
    }

    enum class ModeWord { None, Major, Minor, Invalid };

    /** Ein einzelnes "m" gilt nur klein geschrieben als Moll. "FM" ist in
        Dateinamen mehrdeutig (F-Dur? FM-Synthese?) und wird verworfen.
    */
    ModeWord parseModeWord (const std::string& word)
    {
        if (word.empty())
            return ModeWord::None;

        if (word == "m")
            return ModeWord::Minor;

        const auto lower = toLower (word);

        if (lower == "min" || lower == "minor" || lower == "moll")
            return ModeWord::Minor;

        if (lower == "maj" || lower == "major" || lower == "dur")
            return ModeWord::Major;

        return ModeWord::Invalid;
    }

    /** Sample-Packs schreiben das Tempo oft ohne "bpm" hinter eines dieser
        Woerter: "Ghosthack Bass Loop_120_Decay Bass_E Minor". Eine blosse
        Zahl reicht als Kennzeichen nicht — "149_5.wav" ist kein Tempo.
    */
    bool isTempoWord (const std::string& lower)
    {
        return lower == "loop" || lower == "tempo";
    }

    struct NoteToken
    {
        int pitchClass;
        ModeWord mode;
    };

    /** Deutet ein Wort als Ton, optional mit Vorzeichen und Tongeschlecht. */
    std::optional<NoteToken> parseNoteToken (const std::string& token)
    {
        if (token.empty())
            return std::nullopt;

        static const int basePitch[7] { 9, 11, 0, 2, 4, 5, 7 };    // A B C D E F G

        const char letter = static_cast<char> (std::toupper (static_cast<unsigned char> (token[0])));
        if (letter < 'A' || letter > 'G')
            return std::nullopt;

        int pitchClass = basePitch[letter - 'A'];
        size_t pos = 1;

        const auto rest = toLower (token.substr (1));

        if (rest.rfind ("sharp", 0) == 0)          { pitchClass += 1; pos += 5; }
        else if (rest.rfind ("flat", 0) == 0)      { pitchClass -= 1; pos += 4; }
        else if (pos < token.size() && token[pos] == '#') { pitchClass += 1; pos += 1; }
        else if (pos < token.size() && token[pos] == 'b') { pitchClass -= 1; pos += 1; }

        const auto mode = parseModeWord (token.substr (pos));
        if (mode == ModeWord::Invalid)
            return std::nullopt;

        return NoteToken { wrap (pitchClass, 12), mode };
    }
}

int Key::index() const noexcept
{
    return tonic + (mode == Mode::Minor ? 12 : 0);
}

Key Key::fromIndex (int index) noexcept
{
    const int i = wrap (index, 24);
    return { i % 12, i < 12 ? Mode::Major : Mode::Minor };
}

Key Key::relative() const noexcept
{
    if (mode == Mode::Major)
        return { wrap (tonic + 9, 12), Mode::Minor };

    return { wrap (tonic + 3, 12), Mode::Major };
}

std::string Key::name() const
{
    return std::string (noteName (tonic)) + (mode == Mode::Major ? " Major" : " Minor");
}

std::string Key::shortName() const
{
    return std::string (noteName (tonic)) + (mode == Mode::Major ? "" : "m");
}

const char* noteName (int pitchClass) noexcept
{
    return noteNames[wrap (pitchClass, 12)];
}

KeyRelation relationOf (const Key& estimated, const Key& reference) noexcept
{
    if (estimated == reference)
        return KeyRelation::Same;

    if (estimated.mode == reference.mode)
    {
        const int distance = wrap (estimated.tonic - reference.tonic, 12);
        if (distance == 7 || distance == 5)
            return KeyRelation::Fifth;
    }

    if (estimated == reference.relative())
        return KeyRelation::Relative;

    if (estimated.tonic == reference.tonic)
        return KeyRelation::Parallel;

    return KeyRelation::Other;
}

double mirexScore (const Key& estimated, const Key& reference) noexcept
{
    switch (relationOf (estimated, reference))
    {
        case KeyRelation::Same:     return 1.0;
        case KeyRelation::Fifth:    return 0.5;
        case KeyRelation::Relative: return 0.3;
        case KeyRelation::Parallel: return 0.2;
        case KeyRelation::Other:    break;
    }

    return 0.0;
}

const char* relationName (KeyRelation relation) noexcept
{
    switch (relation)
    {
        case KeyRelation::Same:     return "exakt";
        case KeyRelation::Fifth:    return "Quinte";
        case KeyRelation::Relative: return "Parallele";
        case KeyRelation::Parallel: return "Gleichnamig";
        case KeyRelation::Other:    break;
    }

    return "andere";
}

std::optional<Key> parseKey (const std::string& text)
{
    const auto tokens = tokenize (text);
    std::optional<Key> found;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const auto note = parseNoteToken (tokens[i]);
        if (! note)
            continue;

        auto mode = note->mode;

        // Getrennt geschrieben: "F# minor", "C_Major".
        if (mode == ModeWord::None && i + 1 < tokens.size())
        {
            const auto next = parseModeWord (tokens[i + 1]);
            if (next == ModeWord::Major || next == ModeWord::Minor)
                mode = next;
        }

        if (mode == ModeWord::Major || mode == ModeWord::Minor)
            found = Key { note->pitchClass, mode == ModeWord::Minor ? Mode::Minor : Mode::Major };
    }

    return found;
}

std::optional<double> parseBpm (const std::string& text)
{
    const auto tokens = tokenize (text);
    std::optional<double> found;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const auto lower = toLower (tokens[i]);
        std::string digits;

        if (lower.size() > 3 && lower.compare (lower.size() - 3, 3, "bpm") == 0)
            digits = lower.substr (0, lower.size() - 3);
        else if (lower == "bpm" && i > 0)
            digits = tokens[i - 1];
        else if (i > 0 && isTempoWord (toLower (tokens[i - 1])))
            digits = lower;
        else
            continue;

        if (digits.empty() || digits.size() > 3 || digits.find_first_not_of ("0123456789") != std::string::npos)
            continue;

        const double value = std::stod (digits);
        if (value >= 40.0 && value <= 300.0)
            found = value;
    }

    return found;
}

} // namespace keyy
