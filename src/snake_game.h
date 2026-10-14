#pragma once

#include "game_types.h"

#include <cstddef>
#include <deque>
#include <vector>

// A single square of the board.
class Cell
{
  public:
    Cell(int x, int y);

    Point position() const;

  private:
    Point position_;
};

// Food is linked to the board cell it occupies, which provides its position.
class Food
{
  public:
    explicit Food(const Cell &cell);

    const Cell &cell() const;
    Point position() const;
    void move_to(const Cell &cell);

  private:
    const Cell *cell_;
};

// The snake's body, ordered from head to tail, and its heading.
class Snake
{
  public:
    std::deque<Point> body;
    Direction dir = RIGHT;

    void reset(Point start, Direction direction);
    Point head() const;
    std::size_t length() const;
};

// A grid of cells holding the snake and the food.
class Board
{
  public:
    explicit Board(int width = GRID_WIDTH, int height = GRID_HEIGHT);
    Board(const Board &other);
    Board &operator=(const Board &other);

    int width() const;
    int height() const;
    bool contains(Point p) const;
    const std::vector<Cell> &cells() const;
    // Throws std::out_of_range when p is not on the board.
    const Cell &cell_at(Point p) const;

    Snake &snake();
    const Snake &snake() const;
    const Food &food() const;

    // Moves the food to a random cell not occupied by the snake, if one exists.
    void place_food();
    void place_food_at(Point p);

  private:
    int width_;
    int height_;
    std::vector<Cell> cells_;
    Snake snake_;
    Food food_;
};

class SnakeGame
{
  public:
    Board board;
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
