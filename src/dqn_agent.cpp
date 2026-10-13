#include "dqn_agent.h"

#include <algorithm>
#include <random>

namespace
{
std::mt19937 &random_engine()
{
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
} // namespace

DQNAgent::DQNAgent()
    : policy_net({7, 128, 64, 3}, learning_rate), target_net({7, 128, 64, 3}, learning_rate)
{
    sync_target_network();
}

Action DQNAgent::get_action(const std::vector<float> &state, bool training)
{
    std::uniform_real_distribution<float> probability(0.0f, 1.0f);
    if (training && probability(random_engine()) < epsilon)
    {
        std::uniform_int_distribution<int> action(0, 2);
        return static_cast<Action>(action(random_engine()));
    }

    std::vector<float> q_values = policy_net.forward(state);
    int best_action = 0;
    float best_q = q_values[0];
    for (int i = 1; i < 3; ++i)
    {
        if (q_values[i] > best_q)
        {
            best_q = q_values[i];
            best_action = i;
        }
    }
    return static_cast<Action>(best_action);
}

void DQNAgent::store_transition(const std::vector<float> &state, Action action, float reward,
                                const std::vector<float> &next_state, bool done)
{
    replay_buffer.push_back({state, action, reward, next_state, done});
    if (replay_buffer.size() > REPLAY_CAPACITY)
    {
        replay_buffer.pop_front();
    }
}

void DQNAgent::sync_target_network()
{
    target_net.weights = policy_net.weights;
    target_net.biases = policy_net.biases;
}

void DQNAgent::train()
{
    if (replay_buffer.size() < BATCH_SIZE) return;

    std::vector<size_t> sample_indices;
    sample_indices.reserve(BATCH_SIZE);
    std::uniform_int_distribution<size_t> sample(0, replay_buffer.size() - 1);
    for (size_t i = 0; i < BATCH_SIZE; ++i)
    {
        sample_indices.push_back(sample(random_engine()));
    }

    for (size_t idx : sample_indices)
    {
        const auto &transition = replay_buffer[idx];
        std::vector<float> q_values = policy_net.forward(transition.state);
        std::vector<float> target_q = q_values;

        if (transition.done)
        {
            target_q[transition.action] = transition.reward;
        }
        else
        {
            std::vector<float> next_q = target_net.forward(transition.next_state);
            float max_next_q = *std::max_element(next_q.begin(), next_q.end());
            target_q[transition.action] = transition.reward + gamma * max_next_q;
        }

        policy_net.train(transition.state, target_q);
    }

    ++update_counter;
    if (update_counter % TARGET_UPDATE_INTERVAL == 0)
    {
        sync_target_network();
    }
}

void DQNAgent::decay_epsilon()
{
    if (epsilon > epsilon_min)
    {
        epsilon *= epsilon_decay;
    }
}