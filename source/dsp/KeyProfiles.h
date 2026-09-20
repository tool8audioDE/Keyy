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
enum class Profile { Krumhansl, Temperley, Shaath, Edm, Mix };

struct KeyProfile
{
    const char* name;
    std::array<double, 12> major;
    std::array<double, 12> minor;
};

const KeyProfile& getProfile (Profile profile) noexcept;

/** "krumhansl", "temperley", "shaath", "edm", "mix" */
std::optional<Profile> profileFromName (const std::string& name);

constexpr std::array<Profile, 5> allProfiles { Profile::Krumhansl, Profile::Temperley, Profile::Shaath,
                                              Profile::Edm, Profile::Mix };

/** `mix`, weil das Material des Nutzers Trap und Hip-Hop ist: auf dessen
    Testdateien 61,7 % exakt gegen 55,0 % für `edm`, auf Hip-Hop-,
    Cinematic- und Klavier-Packs durchgehend 4 bis 14 Punkte besser.

    Wer Techno oder House analysiert, nimmt `edm` — dort ist `mix` um 9 bis
    29 Punkte schlechter. Die Profilwahl ist genreabhängig; ein Profil, das
    beides kann, gibt es nach jetzigem Stand nicht.
*/
constexpr Profile defaultProfile = Profile::Mix;

} // namespace keyy
