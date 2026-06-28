#pragma once

#include <cmath>

namespace haystack
{

inline constexpr double kScoreTieEpsilon = 1e-9;

inline bool scores_tied(double a, double b)
{
    return std::abs(a - b) < kScoreTieEpsilon;
}

// Sort key: higher score first; tied scores (|Δ| < 1e-9) break by ascending docId.
inline bool compare_scored_desc(int doc_a, double score_a, int doc_b, double score_b)
{
    if (!scores_tied(score_a, score_b))
        return score_a > score_b;
    return doc_a < doc_b;
}

} // namespace haystack
