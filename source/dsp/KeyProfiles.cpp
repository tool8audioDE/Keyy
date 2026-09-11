#include "KeyProfiles.h"

namespace keyy
{

namespace
{
    // Krumhansl & Kessler (1982): Hörversuche, wie gut ein Ton nach einer
    // Kadenz "passt". Das klassische Profil, Grundlage fast aller Verfahren.
    const KeyProfile krumhansl {
        "krumhansl",
        { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 },
        { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 } };

    // Temperley (2007), gezählt im Kostka-Payne-Korpus: Wie oft ein Ton in
    // einem Abschnitt der Tonart vorkommt. Schärfer als Krumhansl —
    // leitereigene Töne hoch, leiterfremde fast null.
    const KeyProfile temperley {
        "temperley",
        { 0.748, 0.060, 0.488, 0.082, 0.670, 0.460, 0.096, 0.715, 0.104, 0.366, 0.057, 0.400 },
        { 0.712, 0.084, 0.474, 0.618, 0.049, 0.460, 0.105, 0.747, 0.404, 0.067, 0.133, 0.330 } };

    // Sha'ath (2011, KeyFinder): an Pop- und elektronischer Musik aus Audio
    // angepasst. Gegenüber Krumhansl vor allem die kleine Septime in Moll
    // angehoben — natürliches Moll ist dort häufiger als harmonisches.
    // Werte übernommen wie in Essentia (Profil "shaath").
    const KeyProfile shaath {
        "shaath",
        { 6.6, 2.0, 3.5, 2.3, 4.6, 4.0, 2.5, 5.2, 2.4, 3.7, 2.3, 3.4 },
        { 6.5, 2.7, 3.5, 5.4, 2.6, 3.5, 2.5, 5.2, 4.0, 2.7, 4.3, 3.2 } };

    // Aus Keyys eigenem Chromagramm gelernt: Mittel über 301 Stücke des
    // GiantSteps-Datensatzes (EDM), jeweils auf den beschrifteten Grundton
    // gedreht. Die Lehrbuch-Profile beschreiben, welche Töne eine Tonart
    // ausmachen; dieses beschreibt, wie das Chromagramm dieser Analyse bei
    // echter Musik tatsächlich aussieht — mit Obertönen und Rauschboden.
    // Kreuzvalidiert (lernen auf der einen Hälfte, prüfen auf der anderen):
    // 54,5 % exakt. Das Dur-Profil stützt sich auf nur 44 Stücke.
    // Eingestellt: Schwelle 2, Exponent 0,5, Tonalitätsgewichtung 1 — bei
    // anderen Einstellungen muss es neu gelernt werden.
    const KeyProfile edm {
        "edm",
        { 11.20, 6.07, 9.55, 6.71, 10.03, 8.38, 7.19, 11.31, 6.31, 8.70, 6.48, 8.08 },
        { 12.23, 6.99, 8.51, 9.05, 7.32, 8.25, 6.42, 10.69, 7.17, 6.99, 9.03, 7.35 } };
}

const KeyProfile& getProfile (Profile profile) noexcept
{
    switch (profile)
    {
        case Profile::Krumhansl: return krumhansl;
        case Profile::Temperley: return temperley;
        case Profile::Shaath:    return shaath;
        case Profile::Edm:       break;
    }

    return edm;
}

std::optional<Profile> profileFromName (const std::string& name)
{
    for (const auto profile : allProfiles)
        if (name == getProfile (profile).name)
            return profile;

    return std::nullopt;
}

} // namespace keyy
