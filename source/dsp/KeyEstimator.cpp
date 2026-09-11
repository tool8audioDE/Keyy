#include "KeyEstimator.h"

#include <cmath>

#include "DspCommon.h"

namespace keyy
{

namespace
{
    double correlation (const std::array<double, 12>& a, const std::array<double, 12>& b) noexcept
    {
        double meanA = 0.0, meanB = 0.0;
        for (size_t i = 0; i < 12; ++i)
        {
            meanA += a[i];
            meanB += b[i];
        }
        meanA /= 12.0;
        meanB /= 12.0;

        double covariance = 0.0, varianceA = 0.0, varianceB = 0.0;
        for (size_t i = 0; i < 12; ++i)
        {
            const double da = a[i] - meanA;
            const double db = b[i] - meanB;
            covariance += da * db;
            varianceA  += da * da;
            varianceB  += db * db;
        }

        if (varianceA <= 0.0 || varianceB <= 0.0)
            return 0.0;

        return covariance / std::sqrt (varianceA * varianceB);
    }
}

KeyEstimate estimateKey (const std::array<double, 12>& chroma, Profile profile)
{
    const auto& shapes = getProfile (profile);
    KeyEstimate estimate;

    int best = -1, second = -1;

    for (int index = 0; index < 24; ++index)
    {
        const Key key = Key::fromIndex (index);
        const auto& shape = key.mode == Mode::Major ? shapes.major : shapes.minor;

        // Profil auf den Grundton drehen: Tonklasse pc liegt im Profil an
        // der Stelle "Abstand vom Grundton".
        std::array<double, 12> rotated {};
        for (int pc = 0; pc < 12; ++pc)
            rotated[static_cast<size_t> (pc)] = shape[static_cast<size_t> (wrap (pc - key.tonic, 12))];

        const double score = correlation (chroma, rotated);
        estimate.scores[static_cast<size_t> (index)] = score;

        if (best < 0 || score > estimate.scores[static_cast<size_t> (best)])
        {
            second = best;
            best = index;
        }
        else if (second < 0 || score > estimate.scores[static_cast<size_t> (second)])
        {
            second = index;
        }
    }

    estimate.key           = Key::fromIndex (best);
    estimate.score         = estimate.scores[static_cast<size_t> (best)];
    estimate.runnerUp      = Key::fromIndex (second);
    estimate.runnerUpScore = estimate.scores[static_cast<size_t> (second)];

    return estimate;
}

} // namespace keyy
