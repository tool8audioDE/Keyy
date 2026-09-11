#pragma once

#include <array>

#include "Key.h"
#include "KeyProfiles.h"

namespace keyy
{

struct KeyEstimate
{
    Key    key;
    double score = 0.0;             ///< Korrelation, -1..1
    Key    runnerUp;
    double runnerUpScore = 0.0;
    std::array<double, 24> scores {};   ///< je Tonart, Index wie Key::index()
};

/** Vergleicht ein Chromagramm mit allen 24 Tonarten eines Profils.

    Maß ist die Pearson-Korrelation zwischen Chromagramm und dem auf den
    jeweiligen Grundton gedrehten Profil. Sie ist unabhängig von Lautstärke
    und Gleichanteil: Es zählt nur, welche Töne stärker vertreten sind als
    andere, nicht wie stark insgesamt.
*/
KeyEstimate estimateKey (const std::array<double, 12>& chroma, Profile profile);

} // namespace keyy
