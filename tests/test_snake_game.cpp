#include "test_helpers.h"

#include "../src/game_types.h"
#include "../src/snake_game.h"

#include <stdexcept>

static void test_reset()
{
    SnakeGame g;
    g.score = 5;
    g.game_over = true;
    g.reset();
    CHECK(g.board.snake().body.size() == 1);
    CHECK(g.board.snake().body.front() == (Point{GRID_WIDTH / 2, GRID_HEIGHT / 2}));
    CHECK(g.board.snake().dir == RIGHT);
    CHECK(g.score == 0);
    CHECK(g.steps_without_food == 0);
    CHECK(!g.game_over);
    CHECK(!(g.board.food().position() == g.board.snake().body.front()));
}

static void test_place_food()
{
    SnakeGame g;
    g.board.snake().body.clear();
    for (int x = 0; x < GRID_WIDTH; ++x)
        for (int y = 0; y < GRID_HEIGHT; ++y)
            if (!(x == 3 && y == 4)) g.board.snake().body.push_back({x, y});
    g.place_food();
    CHECK(g.board.food().position() == (Point{3, 4}));

    for (int i = 0; i < 100; ++i)
    {
        SnakeGame h;
        h.place_food();
        CHECK(h.board.food().position().x >= 0 && h.board.food().position().x < GRID_WIDTH);
        CHECK(h.board.food().position().y >= 0 && h.board.food().position().y < GRID_HEIGHT);
        CHECK(!(h.board.food().position() == h.board.snake().body.front()));
    }
}

static void test_is_collision()
{
    SnakeGame g;
    CHECK(g.is_collision({-1, 0}));
    CHECK(g.is_collision({0, -1}));
    CHECK(g.is_collision({GRID_WIDTH, 0}));
    CHECK(g.is_collision({0, GRID_HEIGHT}));
    CHECK(!g.is_collision({5, 5}));

    g.board.snake().body.clear();
    g.board.snake().body = {{5, 5}, {4, 5}, {3, 5}};
    CHECK(g.is_collision({4, 5}));
    // Tail will move away unless food is eaten.
    CHECK(!g.is_collision({3, 5}, false));
    CHECK(g.is_collision({3, 5}, true));
}

static void test_step_movement()
{
    SnakeGame g;
    g.board.place_food_at({0, 0});
    Point start = g.board.snake().body.front();
    float r = g.step(STRAIGHT);
    CHECK(r == 0.0f);
    CHECK(g.board.snake().body.front() == (Point{start.x + 1, start.y}));
    CHECK(g.board.snake().body.size() == 1);
    CHECK(g.steps_without_food == 1);

    g.step(TURN_RIGHT);
    CHECK(g.board.snake().dir == DOWN);
    g.step(TURN_LEFT);
    CHECK(g.board.snake().dir == RIGHT);
    g.step(TURN_LEFT);
    CHECK(g.board.snake().dir == UP);
    g.step(TURN_LEFT);
    CHECK(g.board.snake().dir == LEFT);
}

static void test_step_food_and_death()
{
    SnakeGame g;
    g.board.place_food_at({g.board.snake().body.front().x + 1, g.board.snake().body.front().y});
    float r = g.step(STRAIGHT);
    CHECK(r == 10.0f);
    CHECK(g.score == 1);
    CHECK(g.board.snake().body.size() == 2);
    CHECK(g.steps_without_food == 0);
    CHECK(!g.game_over);

    SnakeGame w;
    w.board.snake().body.clear();
    w.board.snake().body.push_back({GRID_WIDTH - 1, 0});
    w.board.snake().dir = RIGHT;
    w.board.place_food_at({0, 0});
    CHECK(w.step(STRAIGHT) == -10.0f);
    CHECK(w.game_over);

    SnakeGame s;
    s.board.place_food_at({0, 0});
    s.steps_without_food = GRID_WIDTH * GRID_HEIGHT * 2 + 1;
    CHECK(s.step(STRAIGHT) == -10.0f);
    CHECK(s.game_over);
}

static void test_state_vector()
{
    SnakeGame g;
    g.board.snake().body.clear();
    g.board.snake().body.push_back({0, 0});
    g.board.snake().dir = RIGHT;
    g.board.place_food_at({5, 0});
    auto s = g.get_state_vector();
    CHECK(s.size() == 7);
    // forward, right(y+1), left(y-1 -> wall)
    CHECK(s[0] == 0.0f);
    CHECK(s[1] == 0.0f);
    CHECK(s[2] == 1.0f);
    CHECK(s[3] == 1.0f); // food ahead
    CHECK(s[4] == 0.0f);
    CHECK(s[5] == 0.0f);
    CHECK(s[6] == 0.0f);

    g.board.snake().dir = LEFT;
    s = g.get_state_vector();
    CHECK(s[0] == 1.0f); // wall ahead
    CHECK(s[6] == 1.0f); // food behind

    g.board.snake().dir = UP;
    g.board.place_food_at({3, 3});
    s = g.get_state_vector();
    CHECK(s[4] == 1.0f); // food to the right (x>0)
    CHECK(s[6] == 1.0f); // and behind (y>0)

    g.board.snake().dir = DOWN;
    s = g.get_state_vector();
    CHECK(s[3] == 1.0f);
    CHECK(s[5] == 1.0f); // dx>0 is left when facing down
}

static void test_board()
{
    Board board(4, 3);
    CHECK(board.width() == 4);
    CHECK(board.height() == 3);
    CHECK(board.cells().size() == 12);
    for (const Cell &cell : board.cells())
        CHECK(&board.cell_at(cell.position()) == &cell);
    CHECK(board.cell_at({3, 2}).position() == (Point{3, 2}));
    CHECK(board.contains({0, 0}) && board.contains({3, 2}));
    CHECK(!board.contains({4, 0}) && !board.contains({0, 3}) && !board.contains({-1, 0}));
    bool threw = false;
    try
    {
        board.cell_at({4, 0});
    }
    catch (const std::out_of_range &)
    {
        threw = true;
    }
    CHECK(threw);

    // Food is linked to one of the board's own cells.
    board.place_food_at({2, 1});
    CHECK(&board.food().cell() == &board.cell_at({2, 1}));
    CHECK(board.food().position() == (Point{2, 1}));

    Board copy(board);
    CHECK(copy.food().position() == (Point{2, 1}));
    CHECK(&copy.food().cell() == &copy.cell_at({2, 1}));
    Board assigned;
    assigned = board;
    CHECK(assigned.width() == 4 && assigned.cells().size() == 12);
    CHECK(&assigned.food().cell() == &assigned.cell_at({2, 1}));

    SnakeGame game;
    game.board.place_food_at({1, 1});
    SnakeGame game_copy = game;
    CHECK(&game_copy.board.food().cell() == &game_copy.board.cell_at({1, 1}));
}

int main()
{
    test_reset();
    test_board();
    test_place_food();
    test_is_collision();
    test_step_movement();
    test_step_food_and_death();
    test_state_vector();
    return TEST_RESULT();
}
