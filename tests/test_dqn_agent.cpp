#include "test_helpers.h"

#include "../src/dqn_agent.h"

#include <cmath>

static const std::vector<float> S = {1, 0, 0, 1, 0, 0, 0};

static void test_init() {
    DQNAgent a;
    CHECK(std::fabs(a.epsilon - 1.0f) < 1e-5f);
    CHECK(std::fabs(a.policy_net.learning_rate - a.learning_rate) < 1e-5f);
    CHECK(a.policy_net.weights == a.target_net.weights);
    CHECK(a.policy_net.biases == a.target_net.biases);
    CHECK(a.policy_net.forward(S).size() == 3);
}

static void test_get_action() {
    DQNAgent a;
    auto q = a.policy_net.forward(S);
    int best = 0;
    for (int i = 1; i < 3; ++i)
        if (q[i] > q[best]) best = i;
    CHECK(a.get_action(S, false) == static_cast<Action>(best));

    a.epsilon = 0.0f;
    CHECK(a.get_action(S, true) == static_cast<Action>(best));

    a.epsilon = 1.0f;
    for (int i = 0; i < 50; ++i) {
        int act = a.get_action(S, true);
        CHECK(act >= 0 && act <= 2);
    }
}

static void test_replay_buffer() {
    DQNAgent a;
    for (size_t i = 0; i < a.REPLAY_CAPACITY + 10; ++i)
        a.store_transition(S, STRAIGHT, static_cast<float>(i), S, false);
    CHECK(a.replay_buffer.size() == a.REPLAY_CAPACITY);
    CHECK(a.replay_buffer.front().reward == 10.0f);
}

static void test_train_and_sync() {
    DQNAgent a;
    a.train();  // too few samples: no-op
    CHECK(a.update_counter == 0);

    for (size_t i = 0; i < a.BATCH_SIZE; ++i)
        a.store_transition(S, TURN_RIGHT, 1.0f, S, i % 2 == 0);
    auto before = a.policy_net.weights;
    a.train();
    CHECK(a.update_counter == 1);
    CHECK(a.policy_net.weights != before);
    CHECK(a.policy_net.weights != a.target_net.weights);

    for (int i = 1; i < a.TARGET_UPDATE_INTERVAL; ++i) a.train();
    CHECK(a.update_counter == a.TARGET_UPDATE_INTERVAL);
    CHECK(a.policy_net.weights == a.target_net.weights);
}

static void test_decay_epsilon() {
    DQNAgent a;
    a.decay_epsilon();
    CHECK(a.epsilon < 1.0f);
    a.epsilon = a.epsilon_min;
    a.decay_epsilon();
    CHECK(a.epsilon == a.epsilon_min);
    for (int i = 0; i < 10000; ++i) a.decay_epsilon();
    CHECK(a.epsilon > 0.0f);
}

int main() {
    test_init();
    test_get_action();
    test_replay_buffer();
    test_train_and_sync();
    test_decay_epsilon();
    return TEST_RESULT();
}
