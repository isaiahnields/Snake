#include "snake_game.h"

#include <random>

namespace
{
std::mt19937 &random_engine()
{
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
} // namespace

SnakeGame::SnakeGame()
{
    reset();
}

void SnakeGame::reset()
{
    snake.clear();
    snake.push_back({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    dir = RIGHT;
    score = 0;
    steps_without_food = 0;
    game_over = false;
    place_food();
}

void SnakeGame::place_food()
{
    if (snake.size() >= GRID_WIDTH * GRID_HEIGHT) return;
    std::uniform_int_distribution<int> x_distribution(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<int> y_distribution(0, GRID_HEIGHT - 1);
    while (true)
    {
        food.x = x_distribution(random_engine());
        food.y = y_distribution(random_engine());
        bool on_snake = false;
        for (const auto &p : snake)
        {
            if (p == food)
            {
                on_snake = true;
                break;
            }
        }
        if (!on_snake) break;
    }
}

bool SnakeGame::is_collision(Point p, bool is_food_at_p)
{
    if (p.x < 0 || p.x >= GRID_WIDTH || p.y < 0 || p.y >= GRID_HEIGHT) return true;
    size_t limit = is_food_at_p ? snake.size() : snake.size() - 1;
    for (size_t i = 0; i < limit; ++i)
    {
        if (snake[i] == p) return true;
    }
    return false;
}

std::vector<float> SnakeGame::get_state_vector()
{
    Point head = snake.front();
    Point p_fwd = head, p_right = head, p_left = head;

    if (dir == UP)
    {
        p_fwd.y -= 1;
        p_right.x += 1;
        p_left.x -= 1;
    }
    else if (dir == RIGHT)
    {
        p_fwd.x += 1;
        p_right.y += 1;
        p_left.y -= 1;
    }
    else if (dir == DOWN)
    {
        p_fwd.y += 1;
        p_right.x -= 1;
        p_left.x += 1;
    }
    else if (dir == LEFT)
    {
        p_fwd.x -= 1;
        p_right.y -= 1;
        p_left.y += 1;
    }

    bool danger_fwd = is_collision(p_fwd, p_fwd == food);
    bool danger_right = is_collision(p_right, p_right == food);
    bool danger_left = is_collision(p_left, p_left == food);

    bool food_fwd = false, food_right = false, food_left = false, food_back = false;
    int dx = food.x - head.x;
    int dy = food.y - head.y;

    if (dir == UP)
    {
        if (dy < 0) food_fwd = true;
        if (dy > 0) food_back = true;
        if (dx > 0) food_right = true;
        if (dx < 0) food_left = true;
    }
    else if (dir == RIGHT)
    {
        if (dx > 0) food_fwd = true;
        if (dx < 0) food_back = true;
        if (dy > 0) food_right = true;
        if (dy < 0) food_left = true;
    }
    else if (dir == DOWN)
    {
        if (dy > 0) food_fwd = true;
        if (dy < 0) food_back = true;
        if (dx < 0) food_right = true;
        if (dx > 0) food_left = true;
    }
    else if (dir == LEFT)
    {
        if (dx < 0) food_fwd = true;
        if (dx > 0) food_back = true;
        if (dy < 0) food_right = true;
        if (dy > 0) food_left = true;
    }

    return {static_cast<float>(danger_fwd),  static_cast<float>(danger_right),
            static_cast<float>(danger_left), static_cast<float>(food_fwd),
            static_cast<float>(food_right),  static_cast<float>(food_left),
            static_cast<float>(food_back)};
}

float SnakeGame::step(Action action)
{
    if (action == TURN_RIGHT)
        dir = static_cast<Direction>((dir + 1) % 4);
    else if (action == TURN_LEFT)
        dir = static_cast<Direction>((dir + 3) % 4);

    Point next_head = snake.front();
    if (dir == UP)
        next_head.y -= 1;
    else if (dir == RIGHT)
        next_head.x += 1;
    else if (dir == DOWN)
        next_head.y += 1;
    else if (dir == LEFT)
        next_head.x -= 1;

    if (is_collision(next_head, next_head == food) ||
        steps_without_food > GRID_WIDTH * GRID_HEIGHT * 2)
    {
        game_over = true;
        return -10.0f;
    }

    snake.push_front(next_head);
    steps_without_food++;

    if (next_head == food)
    {
        score++;
        steps_without_food = 0;
        place_food();
        return 10.0f;
    }

    snake.pop_back();
    return 0.0f;
}