#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <ctime>

const int GRID_WIDTH = 20;
const int GRID_HEIGHT = 20;
const int CELL_SIZE = 30;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };
enum Action { STRAIGHT = 0, TURN_RIGHT = 1, TURN_LEFT = 2 };

// --- 1. THE GAME ENVIRONMENT ---
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
            for (auto& p : snake) {
                if (p == food) { on_snake = true; break; }
            }
            if (!on_snake) break;
        }
    }

    bool is_collision(Point p, bool is_food_at_p = false) {
        if (p.x < 0 || p.x >= GRID_WIDTH || p.y < 0 || p.y >= GRID_HEIGHT) return true;
        // If it's not food, the tail will move forward out of the way, so we ignore the last segment
        size_t limit = is_food_at_p ? snake.size() : snake.size() - 1;
        for (size_t i = 0; i < limit; ++i) {
            if (snake[i] == p) return true;
        }
        return false;
    }

    // Calculates a 7-bit state representation (128 total discrete states)
    unsigned int get_state() {
        Point head = snake.front();
        Point p_fwd = head, p_right = head, p_left = head;
        
        if (dir == UP) { p_fwd.y -= 1; p_right.x += 1; p_left.x -= 1; } 
        else if (dir == RIGHT) { p_fwd.x += 1; p_right.y += 1; p_left.y -= 1; } 
        else if (dir == DOWN) { p_fwd.y += 1; p_right.x -= 1; p_left.x += 1; } 
        else if (dir == LEFT) { p_fwd.x -= 1; p_right.y -= 1; p_left.y += 1; }

        bool danger_fwd = is_collision(p_fwd, p_fwd == food);
        bool danger_right = is_collision(p_right, p_right == food);
        bool danger_left = is_collision(p_left, p_left == food);

        bool food_fwd = false, food_right = false, food_left = false, food_back = false;
        int dx = food.x - head.x;
        int dy = food.y - head.y;

        if (dir == UP) {
            if (dy < 0) food_fwd = true;   if (dy > 0) food_back = true;
            if (dx > 0) food_right = true; if (dx < 0) food_left = true;
        } else if (dir == RIGHT) {
            if (dx > 0) food_fwd = true;   if (dx < 0) food_back = true;
            if (dy > 0) food_right = true; if (dy < 0) food_left = true;
        } else if (dir == DOWN) {
            if (dy > 0) food_fwd = true;   if (dy < 0) food_back = true;
            if (dx < 0) food_right = true; if (dx > 0) food_left = true;
        } else if (dir == LEFT) {
            if (dx < 0) food_fwd = true;   if (dx > 0) food_back = true;
            if (dy < 0) food_right = true; if (dy > 0) food_left = true;
        }

        unsigned int state = 0;
        if (danger_fwd) state |= (1 << 0);
        if (danger_right) state |= (1 << 1);
        if (danger_left) state |= (1 << 2);
        if (food_fwd) state |= (1 << 3);
        if (food_right) state |= (1 << 4);
        if (food_left) state |= (1 << 5);
        if (food_back) state |= (1 << 6);
        return state;
    }

    float step(Action action) {
        if (action == TURN_RIGHT) dir = static_cast<Direction>((dir + 1) % 4);
        else if (action == TURN_LEFT) dir = static_cast<Direction>((dir + 3) % 4);

        Point next_head = snake.front();
        if (dir == UP) next_head.y -= 1;
        else if (dir == RIGHT) next_head.x += 1;
        else if (dir == DOWN) next_head.y += 1;
        else if (dir == LEFT) next_head.x -= 1;

        // Death by wall, tail, or starvation
        if (is_collision(next_head, next_head == food) || steps_without_food > GRID_WIDTH * GRID_HEIGHT * 2) {
            game_over = true;
            return -10.0f; // Punishment
        }

        snake.push_front(next_head);
        steps_without_food++;

        if (next_head == food) {
            score++;
            steps_without_food = 0;
            place_food();
            return 10.0f; // Reward
        } else {
            snake.pop_back();
            return 0.0f; // Neutral Step
        }
    }
};

// --- 2. Q-LEARNING AGENT ---
class QLearningAgent {
public:
    std::unordered_map<unsigned int, std::array<float, 3>> q_table;
    float learning_rate = 0.1f;
    float discount_factor = 0.9f;
    float epsilon = 1.0f;          // Starts at 100% exploration
    float epsilon_decay = 0.99f;   // Decays random choices slowly after each death
    float epsilon_min = 0.01f;

    Action get_action(unsigned int state) {
        // Explore randomly
        if ((float)rand() / RAND_MAX < epsilon) {
            return static_cast<Action>(rand() % 3); 
        }
        
        // Exploit learned knowledge
        if (q_table.find(state) == q_table.end()) {
            q_table[state] = {0.0f, 0.0f, 0.0f};
        }
        auto& q_values = q_table[state];
        
        float max_q = q_values[0];
        int best_actions[3] = {0};
        int count = 1;

        // Collect all equally best actions to tie-break randomly (fixes early infinite looping)
        if (q_values[1] > max_q) { max_q = q_values[1]; count = 1; best_actions[0] = 1; }
        else if (q_values[1] == max_q) { best_actions[count++] = 1; }

        if (q_values[2] > max_q) { max_q = q_values[2]; count = 1; best_actions[0] = 2; }
        else if (q_values[2] == max_q) { best_actions[count++] = 2; }

        return static_cast<Action>(best_actions[rand() % count]);
    }

    void update_q_value(unsigned int state, Action action, float reward, unsigned int next_state, bool game_over) {
        if (q_table.find(state) == q_table.end()) q_table[state] = {0.0f, 0.0f, 0.0f};
        if (q_table.find(next_state) == q_table.end()) q_table[next_state] = {0.0f, 0.0f, 0.0f};

        float max_next_q = 0.0f;
        if (!game_over) {
            max_next_q = std::max({q_table[next_state][0], q_table[next_state][1], q_table[next_state][2]});
        }
        
        // The Bellman Equation
        q_table[state][static_cast<int>(action)] += learning_rate * (reward + discount_factor * max_next_q - q_table[state][static_cast<int>(action)]);
    }
};

// --- 3. SDL3 RENDERER & LOOP ---
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Snake Q-Learning (SDL3) - Press SPACE to toggle speed", GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, 0, &window, &renderer)) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SnakeGame game;
    QLearningAgent agent;

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
                    delay_ms = render ? 40 : 0; // Turn off delay to train at max CPU speed
                    std::cout << "Rendering: " << (render ? "ON" : "OFF") << std::endl;
                }
            }
        }

        // Q-Learning pipeline
        unsigned int state = game.get_state();
        Action action = agent.get_action(state);
        float reward = game.step(action);
        unsigned int next_state = game.get_state();

        agent.update_q_value(state, action, reward, next_state, game.game_over);

        if (game.game_over) {
            if (agent.epsilon > agent.epsilon_min) agent.epsilon *= agent.epsilon_decay;
            if (episode % 50 == 0) {
                std::cout << "Episode: " << episode << " | Score: " << game.score << " | Exploration rate: " << agent.epsilon << std::endl;
            }
            game.reset();
            episode++;
        }

        // SDL3 Drawing Logic
        if (render) {
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);

            // Food (Red)
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
            SDL_FRect food_rect = { (float)(game.food.x * CELL_SIZE), (float)(game.food.y * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
            SDL_RenderFillRect(renderer, &food_rect);

            // Snake (Green)
            for (size_t i = 0; i < game.snake.size(); ++i) {
                if (i == 0) SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
                else SDL_SetRenderDrawColor(renderer, 30, 150, 30, 255);

                SDL_FRect snake_rect = {
                    (float)(game.snake[i].x * CELL_SIZE + 1), 
                    (float)(game.snake[i].y * CELL_SIZE + 1), 
                    (float)(CELL_SIZE - 2), (float)(CELL_SIZE - 2)
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