#pragma once

#include <vector>

class NeuralNetwork
{
  public:
    std::vector<int> layer_sizes;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;
    float learning_rate;

    NeuralNetwork(std::vector<int> sizes, float learning_rate);

    static float relu(float x);
    static float relu_derivative(float x);
    std::vector<float> forward(const std::vector<float> &input) const;
    void train(const std::vector<float> &input, const std::vector<float> &target);
};