#pragma once

#include "dqn_agent.h"

#include <cstddef>
#include <string>
#include <vector>

constexpr std::size_t HALL_OF_FAME_SIZE = 5;

// State shared between views for the lifetime of the application. Nothing here is persisted.
struct SessionState
{
    DQNAgent agent;
    int training_episodes = 1000;
    bool visual_training = true;
    std::vector<int> best_manual_scores;
    std::vector<int> best_ai_scores;
};

// Inserts score into a descending list capped at HALL_OF_FAME_SIZE entries.
void record_score(std::vector<int> &best_scores, int score);
std::string format_scores(const std::vector<int> &scores);
