#pragma once

#include "game_types.h"
#include "neural_network.h"

#include <deque>
#include <vector>

struct Transition
{
    std::vector<float> state;
    Action action;
    float reward;
    std::vector<float> next_state;
    bool done;
};

class DQNAgent
{
  public:
    float learning_rate = 0.001f;
    NeuralNetwork policy_net;
    NeuralNetwork target_net;
    std::deque<Transition> replay_buffer;
    const size_t REPLAY_CAPACITY = 2000;
    const size_t BATCH_SIZE = 32;
    float gamma = 0.95f;
    float epsilon = 1.0f;
    float epsilon_decay = 0.995f;
    float epsilon_min = 0.05f;
    int update_counter = 0;
    const int TARGET_UPDATE_INTERVAL = 50;

    DQNAgent();

    Action get_action(const std::vector<float> &state, bool training = true);
    void store_transition(const std::vector<float> &state, Action action, float reward,
                          const std::vector<float> &next_state, bool done);
    void sync_target_network();
    void train();
    void decay_epsilon();
};