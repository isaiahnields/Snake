#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>

const int GRID_WIDTH = 20;
const int GRID_HEIGHT = 20;
const int CELL_SIZE = 30;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };
enum Action { STRAIGHT = 0, TURN_RIGHT = 1, TURN_LEFT = 2 };

class NeuralNetwork {
public:
    std::vector<int> layer_sizes;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;
    float learning_rate;

    NeuralNetwork(std::vector<int> sizes, float lr = 0.001f)
        : layer_sizes(std::move(sizes)), learning_rate(lr) {
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

    static float relu(float x) {
        return x > 0.0f ? x : 0.0f;
    }

    static float relu_derivative(float x) {
        return x > 0.0f ? 1.0f : 0.0f;
    }

    std::vector<float> forward(const std::vector<float>& input) const {
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

    void train(const std::vector<float>& input, const std::vector<float>& target) {
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
};

class SnakeGame {
public:
    std::deque<Point> snake;
    Point food;
    Direction dir;
    int score;
    int steps_without_food;
    bool game_over;

    SnakeGame() { reset(); }

    void reset() {
        snake.clear();
        snake.push_back({GRID_WIDTH / 2, GRID_HEIGHT / 2});
        dir = RIGHT;
        score = 0;
        steps_without_food = 0;
        game_over = false;
        place_food();
    }

    void place_food() {
        if (snake.size() >= GRID_WIDTH * GRID_HEIGHT) return;
        while (true) {
            food.x = rand() % GRID_WIDTH;
            food.y = rand() % GRID_HEIGHT;
            bool on_snake = false;
            for (const auto& p : snake) {
                if (p == food) {
                    on_snake = true;
                    break;
                }
            }
            if (!on_snake) break;
        }
    }

    bool is_collision(Point p, bool is_food_at_p = false) {
        if (p.x < 0 || p.x >= GRID_WIDTH || p.y < 0 || p.y >= GRID_HEIGHT) return true;
        size_t limit = is_food_at_p ? snake.size() : snake.size() - 1;
        for (size_t i = 0; i < limit; ++i) {
            if (snake[i] == p) return true;
        }
        return false;
    }

    std::vector<float> get_state_vector() {
        Point head = snake.front();
        Point p_fwd = head, p_right = head, p_left = head;

        if (dir == UP) {
            p_fwd.y -= 1; p_right.x += 1; p_left.x -= 1;
        } else if (dir == RIGHT) {
            p_fwd.x += 1; p_right.y += 1; p_left.y -= 1;
        } else if (dir == DOWN) {
            p_fwd.y += 1; p_right.x -= 1; p_left.x += 1;
        } else if (dir == LEFT) {
            p_fwd.x -= 1; p_right.y -= 1; p_left.y += 1;
        }

        bool danger_fwd = is_collision(p_fwd, p_fwd == food);
        bool danger_right = is_collision(p_right, p_right == food);
        bool danger_left = is_collision(p_left, p_left == food);

        bool food_fwd = false, food_right = false, food_left = false, food_back = false;
        int dx = food.x - head.x;
        int dy = food.y - head.y;

        if (dir == UP) {
            if (dy < 0) food_fwd = true;
            if (dy > 0) food_back = true;
            if (dx > 0) food_right = true;
            if (dx < 0) food_left = true;
        } else if (dir == RIGHT) {
            if (dx > 0) food_fwd = true;
            if (dx < 0) food_back = true;
            if (dy > 0) food_right = true;
            if (dy < 0) food_left = true;
        } else if (dir == DOWN) {
            if (dy > 0) food_fwd = true;
            if (dy < 0) food_back = true;
            if (dx < 0) food_right = true;
            if (dx > 0) food_left = true;
        } else if (dir == LEFT) {
            if (dx < 0) food_fwd = true;
            if (dx > 0) food_back = true;
            if (dy < 0) food_right = true;
            if (dy > 0) food_left = true;
        }

        return {
            static_cast<float>(danger_fwd),
            static_cast<float>(danger_right),
            static_cast<float>(danger_left),
            static_cast<float>(food_fwd),
            static_cast<float>(food_right),
            static_cast<float>(food_left),
            static_cast<float>(food_back)
        };
    }

    float step(Action action) {
        if (action == TURN_RIGHT) dir = static_cast<Direction>((dir + 1) % 4);
        else if (action == TURN_LEFT) dir = static_cast<Direction>((dir + 3) % 4);

        Point next_head = snake.front();
        if (dir == UP) next_head.y -= 1;
        else if (dir == RIGHT) next_head.x += 1;
        else if (dir == DOWN) next_head.y += 1;
        else if (dir == LEFT) next_head.x -= 1;

        if (is_collision(next_head, next_head == food) || steps_without_food > GRID_WIDTH * GRID_HEIGHT * 2) {
            game_over = true;
            return -10.0f;
        }

        snake.push_front(next_head);
        steps_without_food++;

        if (next_head == food) {
            score++;
            steps_without_food = 0;
            place_food();
            return 10.0f;
        }

        snake.pop_back();
        return 0.0f;
    }
};

struct Transition {
    std::vector<float> state;
    Action action;
    float reward;
    std::vector<float> next_state;
    bool done;
};

class DQNAgent {
public:
    NeuralNetwork policy_net;
    NeuralNetwork target_net;
    std::deque<Transition> replay_buffer;
    const size_t REPLAY_CAPACITY = 2000;
    const size_t BATCH_SIZE = 32;
    float learning_rate = 0.001f;
    float gamma = 0.95f;
    float epsilon = 1.0f;
    float epsilon_decay = 0.995f;
    float epsilon_min = 0.05f;
    int update_counter = 0;
    const int TARGET_UPDATE_INTERVAL = 50;

    DQNAgent()
        : policy_net({7, 128, 64, 3}, learning_rate),
          target_net({7, 128, 64, 3}, learning_rate) {
        sync_target_network();
    }

    Action get_action(const std::vector<float>& state) {
        if ((float)rand() / RAND_MAX < epsilon) {
            return static_cast<Action>(rand() % 3);
        }

        std::vector<float> q_values = policy_net.forward(state);
        int best_action = 0;
        float best_q = q_values[0];
        for (int i = 1; i < 3; ++i) {
            if (q_values[i] > best_q) {
                best_q = q_values[i];
                best_action = i;
            }
        }
        return static_cast<Action>(best_action);
    }

    void store_transition(const std::vector<float>& state, Action action, float reward,
                          const std::vector<float>& next_state, bool done) {
        replay_buffer.push_back({state, action, reward, next_state, done});
        if (replay_buffer.size() > REPLAY_CAPACITY) {
            replay_buffer.pop_front();
        }
    }

    void sync_target_network() {
        target_net.weights = policy_net.weights;
        target_net.biases = policy_net.biases;
    }

    void train() {
        if (replay_buffer.size() < BATCH_SIZE) return;

        std::vector<size_t> sample_indices;
        sample_indices.reserve(BATCH_SIZE);
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            sample_indices.push_back(rand() % replay_buffer.size());
        }

        for (size_t idx : sample_indices) {
            const auto& transition = replay_buffer[idx];
            std::vector<float> q_values = policy_net.forward(transition.state);
            std::vector<float> target_q = q_values;

            if (transition.done) {
                target_q[transition.action] = transition.reward;
            } else {
                std::vector<float> next_q = target_net.forward(transition.next_state);
                float max_next_q = *std::max_element(next_q.begin(), next_q.end());
                target_q[transition.action] = transition.reward + gamma * max_next_q;
            }

            policy_net.train(transition.state, target_q);
        }

        ++update_counter;
        if (update_counter % TARGET_UPDATE_INTERVAL == 0) {
            sync_target_network();
        }
    }

    void decay_epsilon() {
        if (epsilon > epsilon_min) {
            epsilon *= epsilon_decay;
        }
    }
};

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Snake DQN (SDL3) - Press SPACE to toggle speed",
                                     GRID_WIDTH * CELL_SIZE,
                                     GRID_HEIGHT * CELL_SIZE,
                                     0,
                                     &window,
                                     &renderer)) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SnakeGame game;
    DQNAgent agent;

    bool quit = false;
    bool render = true;
    int delay_ms = 40;
    int episode = 1;

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) quit = true;
                if (event.key.key == SDLK_SPACE) {
                    render = !render;
                    delay_ms = render ? 40 : 0;
                    std::cout << "Rendering: " << (render ? "ON" : "OFF") << std::endl;
                }
            }
        }

        std::vector<float> state = game.get_state_vector();
        Action action = agent.get_action(state);
        float reward = game.step(action);
        std::vector<float> next_state = game.get_state_vector();

        agent.store_transition(state, action, reward, next_state, game.game_over);
        agent.train();

        if (game.game_over) {
            agent.decay_epsilon();
            if (episode % 50 == 0) {
                std::cout << "Episode: " << episode
                          << " | Score: " << game.score
                          << " | Epsilon: " << agent.epsilon
                          << std::endl;
            }
            game.reset();
            ++episode;
        }

        if (render) {
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
            SDL_FRect food_rect = {
                (float)(game.food.x * CELL_SIZE),
                (float)(game.food.y * CELL_SIZE),
                (float)CELL_SIZE,
                (float)CELL_SIZE
            };
            SDL_RenderFillRect(renderer, &food_rect);

            for (size_t i = 0; i < game.snake.size(); ++i) {
                if (i == 0) {
                    SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 30, 150, 30, 255);
                }

                SDL_FRect snake_rect = {
                    (float)(game.snake[i].x * CELL_SIZE + 1),
                    (float)(game.snake[i].y * CELL_SIZE + 1),
                    (float)(CELL_SIZE - 2),
                    (float)(CELL_SIZE - 2)
                };
                SDL_RenderFillRect(renderer, &snake_rect);
            }

            SDL_RenderPresent(renderer);
            SDL_Delay(delay_ms);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
