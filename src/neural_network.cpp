#include "neural_network.h"

#include <random>
#include <utility>

NeuralNetwork::NeuralNetwork(std::vector<int> sizes, float learning_rate)
    : layer_sizes(std::move(sizes)), learning_rate(learning_rate) {
    std::mt19937 rng(std::random_device{}());
    std::normal_distribution<float> dist(0.0f, 0.1f);

    for (size_t i = 0; i + 1 < layer_sizes.size(); ++i) {
        std::vector<std::vector<float>> layer_weights(layer_sizes[i + 1], std::vector<float>(layer_sizes[i]));
        for (int j = 0; j < layer_sizes[i + 1]; ++j) {
            for (int k = 0; k < layer_sizes[i]; ++k) {
                layer_weights[j][k] = dist(rng);
            }
        }
        weights.push_back(layer_weights);

        biases.push_back(std::vector<float>(layer_sizes[i + 1]));
        for (int j = 0; j < layer_sizes[i + 1]; ++j) {
            biases.back()[j] = dist(rng);
        }
    }
}

float NeuralNetwork::relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

float NeuralNetwork::relu_derivative(float x) {
    return x > 0.0f ? 1.0f : 0.0f;
}

std::vector<float> NeuralNetwork::forward(const std::vector<float>& input) const {
    std::vector<float> current = input;
    for (size_t layer = 0; layer < weights.size(); ++layer) {
        std::vector<float> next(weights[layer].size(), 0.0f);
        for (size_t j = 0; j < weights[layer].size(); ++j) {
            float sum = biases[layer][j];
            for (size_t k = 0; k < current.size(); ++k) {
                sum += current[k] * weights[layer][j][k];
            }
            next[j] = (layer == weights.size() - 1) ? sum : relu(sum);
        }
        current = next;
    }
    return current;
}

void NeuralNetwork::train(const std::vector<float>& input, const std::vector<float>& target) {
    std::vector<float> current = input;
    std::vector<std::vector<float>> activations;
    std::vector<std::vector<float>> pre_activations;
    activations.push_back(input);

    for (size_t layer = 0; layer < weights.size(); ++layer) {
        std::vector<float> pre(weights[layer].size(), 0.0f);
        std::vector<float> next(weights[layer].size(), 0.0f);

        for (size_t j = 0; j < weights[layer].size(); ++j) {
            float sum = biases[layer][j];
            for (size_t k = 0; k < current.size(); ++k) {
                sum += current[k] * weights[layer][j][k];
            }
            pre[j] = sum;
            next[j] = (layer == weights.size() - 1) ? sum : relu(sum);
        }

        pre_activations.push_back(pre);
        activations.push_back(next);
        current = next;
    }

    std::vector<float> output_delta = current;
    for (size_t i = 0; i < output_delta.size(); ++i) {
        output_delta[i] = current[i] - target[i];
    }

    std::vector<std::vector<float>> deltas(weights.size());
    deltas.back() = output_delta;

    for (int layer = static_cast<int>(weights.size()) - 1; layer >= 0; --layer) {
        std::vector<float> layer_delta(weights[layer].size(), 0.0f);

        for (size_t j = 0; j < weights[layer].size(); ++j) {
            float grad = 0.0f;
            if (layer == static_cast<int>(weights.size()) - 1) {
                grad = deltas[layer][j];
            } else {
                for (size_t next_j = 0; next_j < weights[layer + 1].size(); ++next_j) {
                    grad += deltas[layer + 1][next_j] * weights[layer + 1][next_j][j];
                }
                grad *= relu_derivative(pre_activations[layer][j]);
            }

            layer_delta[j] = grad;
            biases[layer][j] -= learning_rate * grad;

            for (size_t k = 0; k < weights[layer][j].size(); ++k) {
                weights[layer][j][k] -= learning_rate * grad * activations[layer][k];
            }
        }

        if (layer > 0) {
            deltas[layer - 1].resize(weights[layer - 1].size(), 0.0f);
            for (size_t j = 0; j < weights[layer - 1].size(); ++j) {
                float grad = 0.0f;
                for (size_t out_j = 0; out_j < weights[layer].size(); ++out_j) {
                    grad += layer_delta[out_j] * weights[layer][out_j][j];
                }
                deltas[layer - 1][j] = grad * relu_derivative(pre_activations[layer - 1][j]);
            }
        }
    }
}