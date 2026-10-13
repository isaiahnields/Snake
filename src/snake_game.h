#pragma once

#include "game_types.h"

#include <deque>
#include <vector>

class SnakeGame
{
  public:
    std::deque<Point> snake;
    Point food;
    Direction dir;
    int score;
    int steps_without_food;
    bool game_over;

    SnakeGame();

    void reset();
    void place_food();
    bool is_collision(Point p, bool is_food_at_p = false);
    std::vector<float> get_state_vector();
    float step(Action action);
};