#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#include "WavIO.h"
#include "minimp3_ex.h"

/** Lädt WAV und MP3 für das Offline-Werkzeug.

    Das Plugin liest über JUCE auch AIFF, FLAC und Ogg. Das CLI bleibt ohne
    JUCE und kann deshalb nur, was hier steht: WAV aus dem eigenen Leser,
    MP3 über minimp3 — die beiden Formate, in denen Sample-Packs und der
    GiantSteps-Datensatz vorliegen.
*/
namespace keyy::audio
{

static_assert (std::is_same_v<mp3d_sample_t, float>, "MINIMP3_FLOAT_OUTPUT muss vor dem Einbinden definiert sein");

inline std::string lowerExtension (const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    std::transform (ext.begin(), ext.end(), ext.begin(), [] (unsigned char c) { return static_cast<char> (std::tolower (c)); });
    return ext;
}

inline bool isSupported (const std::filesystem::path& path)
{
    const auto ext = lowerExtension (path);
    return ext == ".wav" || ext == ".mp3";
}

inline wav::AudioFile loadMp3 (const std::filesystem::path& path, std::string& error)
{
    wav::AudioFile result;

    // Selbst einlesen statt mp3dec_load(): Das nimmt nur char-Pfade und
    // scheitert unter Windows an Umlauten im Dateinamen.
    std::ifstream file (path, std::ios::binary);
    if (! file)
    {
        error = "Datei nicht lesbar";
        return result;
    }

    const std::vector<uint8_t> bytes ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char>());

    mp3dec_t decoder;
    mp3dec_file_info_t info {};

    if (mp3dec_load_buf (&decoder, bytes.data(), bytes.size(), &info, nullptr, nullptr) != 0
        || info.samples == 0 || info.channels <= 0)
    {
        std::free (info.buffer);
        error = "MP3 nicht dekodierbar";
        return result;
    }

    const auto numChannels = static_cast<size_t> (info.channels);
    const size_t numFrames = info.samples / numChannels;

    result.sampleRate = info.hz;
    result.channels.assign (numChannels, std::vector<float> (numFrames, 0.0f));

    for (size_t frame = 0; frame < numFrames; ++frame)
        for (size_t ch = 0; ch < numChannels; ++ch)
            result.channels[ch][frame] = info.buffer[frame * numChannels + ch];

    std::free (info.buffer);
    return result;
}

inline wav::AudioFile load (const std::filesystem::path& path, std::string& error)
{
    const auto ext = lowerExtension (path);

    if (ext == ".wav")
        return wav::load (path, error);

    if (ext == ".mp3")
        return loadMp3 (path, error);

    error = "Format nicht unterstuetzt (das CLI liest WAV und MP3)";
    return {};
}

} // namespace keyy::audio
