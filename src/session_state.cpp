#include "session_state.h"

#include <algorithm>
#include <functional>

void record_score(std::vector<int> &best_scores, int score)
{
    best_scores.insert(
        std::upper_bound(best_scores.begin(), best_scores.end(), score, std::greater<int>()),
        score);
    if (best_scores.size() > HALL_OF_FAME_SIZE) best_scores.resize(HALL_OF_FAME_SIZE);
}

std::string format_scores(const std::vector<int> &scores)
{
    std::string text;
    for (const int score : scores)
    {
        if (!text.empty()) text += ", ";
        text += std::to_string(score);
    }
    return text;
}
