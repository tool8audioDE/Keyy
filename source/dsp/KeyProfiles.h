#pragma once

#include <array>
#include <optional>
#include <string>

namespace keyy
{

/** Tonart-Profile: wie stark jede der zwölf Tonklassen in einer Tonart
    vertreten ist, vom Grundton aus gezählt.

    Welches Profil auf dem eigenen Material am besten trifft, ist eine
    Messfrage und keine Geschmacksfrage. keyy-cli --batch --profile all
    wertet alle auf einmal aus.
*/
enum class Profile { Krumhansl, Temperley, Shaath, Edm };

struct KeyProfile
{
    const char* name;
    std::array<double, 12> major;
    std::array<double, 12> minor;
};

const KeyProfile& getProfile (Profile profile) noexcept;

/** "krumhansl", "temperley", "shaath", "edm" */
std::optional<Profile> profileFromName (const std::string& name);

constexpr std::array<Profile, 4> allProfiles { Profile::Krumhansl, Profile::Temperley, Profile::Shaath, Profile::Edm };

/** Vorläufig das aus dem eigenen Chromagramm gelernte Profil: auf
    GiantSteps kreuzvalidiert 54,5 % exakt gegen 46,5 % für Sha'ath. Ob das
    auf Trap-Loops und Beats des Nutzers ebenso gilt, entscheiden dessen
    Testdateien.
*/
constexpr Profile defaultProfile = Profile::Edm;

} // namespace keyy
