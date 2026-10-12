#include "test_helpers.h"

#include "../src/neural_network.h"

#include <cmath>

static void test_relu() {
    CHECK(NeuralNetwork::relu(2.5f) == 2.5f);
    CHECK(NeuralNetwork::relu(-1.0f) == 0.0f);
    CHECK(NeuralNetwork::relu(0.0f) == 0.0f);
    CHECK(NeuralNetwork::relu_derivative(2.0f) == 1.0f);
    CHECK(NeuralNetwork::relu_derivative(-2.0f) == 0.0f);
    CHECK(NeuralNetwork::relu_derivative(0.0f) == 0.0f);
}

static void test_construction() {
    NeuralNetwork nn({3, 5, 2}, 0.01f);
    CHECK(nn.learning_rate == 0.01f);
    CHECK(nn.weights.size() == 2);
    CHECK(nn.biases.size() == 2);
    CHECK(nn.weights[0].size() == 5 && nn.weights[0][0].size() == 3);
    CHECK(nn.weights[1].size() == 2 && nn.weights[1][0].size() == 5);
    CHECK(nn.biases[0].size() == 5 && nn.biases[1].size() == 2);
}

static void test_forward_known_weights() {
    NeuralNetwork nn({2, 2, 1});
    nn.weights = {{{1.0f, -1.0f}, {0.5f, 0.5f}}, {{2.0f, 3.0f}}};
    nn.biases = {{0.0f, 0.0f}, {1.0f}};
    // hidden = relu([1-2, 0.5+1]) = [0, 1.5]; out = 3*1.5 + 1 = 5.5
    auto out = nn.forward({1.0f, 2.0f});
    CHECK(out.size() == 1);
    CHECK(std::fabs(out[0] - 5.5f) < 1e-5f);
}

static void test_train_reduces_error() {
    NeuralNetwork nn({2, 8, 1}, 0.01f);
    std::vector<float> in = {0.5f, -0.3f};
    std::vector<float> target = {1.0f};
    float before = std::fabs(nn.forward(in)[0] - target[0]);
    for (int i = 0; i < 500; ++i) nn.train(in, target);
    float after = std::fabs(nn.forward(in)[0] - target[0]);
    CHECK(after < before || after < 1e-3f);
    CHECK(after < 0.05f);
}

int main() {
    test_relu();
    test_construction();
    test_forward_known_weights();
    test_train_reduces_error();
    return TEST_RESULT();
}
