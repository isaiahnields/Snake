#include "snake_game.h"

#include <random>
#include <stdexcept>

namespace
{
std::mt19937 &random_engine()
{
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
} // namespace

Cell::Cell(int x, int y) : position_{x, y} {}

Point Cell::position() const
{
    return position_;
}

Food::Food(const Cell &cell) : cell_(&cell) {}

const Cell &Food::cell() const
{
    return *cell_;
}

Point Food::position() const
{
    return cell_->position();
}

void Food::move_to(const Cell &cell)
{
    cell_ = &cell;
}

void Snake::reset(Point start, Direction direction)
{
    body.clear();
    body.push_back(start);
    dir = direction;
}

Point Snake::head() const
{
    return body.front();
}

std::size_t Snake::length() const
{
    return body.size();
}

namespace
{
std::vector<Cell> make_cells(int width, int height)
{
    std::vector<Cell> cells;
    cells.reserve(static_cast<std::size_t>(width) * height);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            cells.emplace_back(x, y);
    return cells;
}
} // namespace

Board::Board(int width, int height)
    : width_(width), height_(height), cells_(make_cells(width, height)), food_(cells_.front())
{
}

// Food links to a cell of its own board, so copies must relink it to their own cells.
Board::Board(const Board &other)
    : width_(other.width_), height_(other.height_), cells_(other.cells_), snake_(other.snake_),
      food_(cell_at(other.food_.position()))
{
}

Board &Board::operator=(const Board &other)
{
    if (this == &other) return *this;
    width_ = other.width_;
    height_ = other.height_;
    cells_ = other.cells_;
    snake_ = other.snake_;
    food_.move_to(cell_at(other.food_.position()));
    return *this;
}

int Board::width() const
{
    return width_;
}

int Board::height() const
{
    return height_;
}

bool Board::contains(Point p) const
{
    return p.x >= 0 && p.x < width_ && p.y >= 0 && p.y < height_;
}

const std::vector<Cell> &Board::cells() const
{
    return cells_;
}

const Cell &Board::cell_at(Point p) const
{
    if (!contains(p)) throw std::out_of_range("Point is outside the board");
    return cells_[static_cast<std::size_t>(p.y) * width_ + p.x];
}

Snake &Board::snake()
{
    return snake_;
}

const Snake &Board::snake() const
{
    return snake_;
}

const Food &Board::food() const
{
    return food_;
}

void Board::place_food()
{
    if (snake_.length() >= cells_.size()) return;
    std::uniform_int_distribution<int> x_distribution(0, width_ - 1);
    std::uniform_int_distribution<int> y_distribution(0, height_ - 1);
    while (true)
    {
        const Point candidate{x_distribution(random_engine()), y_distribution(random_engine())};
        bool on_snake = false;
        for (const auto &p : snake_.body)
        {
            if (p == candidate)
            {
                on_snake = true;
                break;
            }
        }
        if (on_snake) continue;
        place_food_at(candidate);
        return;
    }
}

void Board::place_food_at(Point p)
{
    food_.move_to(cell_at(p));
}

SnakeGame::SnakeGame()
{
    reset();
}

void SnakeGame::reset()
{
    board.snake().reset({board.width() / 2, board.height() / 2}, RIGHT);
    score = 0;
    steps_without_food = 0;
    game_over = false;
    place_food();
}

void SnakeGame::place_food()
{
    board.place_food();
}

bool SnakeGame::is_collision(Point p, bool is_food_at_p)
{
    if (!board.contains(p)) return true;
    const auto &body = board.snake().body;
    size_t limit = is_food_at_p ? body.size() : body.size() - 1;
    for (size_t i = 0; i < limit; ++i)
    {
        if (body[i] == p) return true;
    }
    return false;
}

std::vector<float> SnakeGame::get_state_vector()
{
    const Snake &snake = board.snake();
    const Point food = board.food().position();
    Point head = snake.head();
    Point p_fwd = head, p_right = head, p_left = head;

    if (snake.dir == UP)
    {
        p_fwd.y -= 1;
        p_right.x += 1;
        p_left.x -= 1;
    }
    else if (snake.dir == RIGHT)
    {
        p_fwd.x += 1;
        p_right.y += 1;
        p_left.y -= 1;
    }
    else if (snake.dir == DOWN)
    {
        p_fwd.y += 1;
        p_right.x -= 1;
        p_left.x += 1;
    }
    else if (snake.dir == LEFT)
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

    if (snake.dir == UP)
    {
        if (dy < 0) food_fwd = true;
        if (dy > 0) food_back = true;
        if (dx > 0) food_right = true;
        if (dx < 0) food_left = true;
    }
    else if (snake.dir == RIGHT)
    {
        if (dx > 0) food_fwd = true;
        if (dx < 0) food_back = true;
        if (dy > 0) food_right = true;
        if (dy < 0) food_left = true;
    }
    else if (snake.dir == DOWN)
    {
        if (dy > 0) food_fwd = true;
        if (dy < 0) food_back = true;
        if (dx < 0) food_right = true;
        if (dx > 0) food_left = true;
    }
    else if (snake.dir == LEFT)
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
    Snake &snake = board.snake();
    if (action == TURN_RIGHT)
        snake.dir = static_cast<Direction>((snake.dir + 1) % 4);
    else if (action == TURN_LEFT)
        snake.dir = static_cast<Direction>((snake.dir + 3) % 4);

    Point next_head = snake.head();
    if (snake.dir == UP)
        next_head.y -= 1;
    else if (snake.dir == RIGHT)
        next_head.x += 1;
    else if (snake.dir == DOWN)
        next_head.y += 1;
    else if (snake.dir == LEFT)
        next_head.x -= 1;

    const bool eats_food = next_head == board.food().position();
    if (is_collision(next_head, eats_food) ||
        steps_without_food > board.width() * board.height() * 2)
    {
        game_over = true;
        return -10.0f;
    }

    snake.body.push_front(next_head);
    steps_without_food++;

    if (eats_food)
    {
        score++;
        steps_without_food = 0;
        place_food();
        return 10.0f;
    }

    snake.body.pop_back();
    return 0.0f;
}
