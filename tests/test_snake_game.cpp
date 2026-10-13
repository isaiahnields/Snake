#include "test_helpers.h"

#include "../src/game_types.h"
#include "../src/snake_game.h"

static void test_reset()
{
    SnakeGame g;
    g.score = 5;
    g.game_over = true;
    g.reset();
    CHECK(g.snake.size() == 1);
    CHECK(g.snake.front() == (Point{GRID_WIDTH / 2, GRID_HEIGHT / 2}));
    CHECK(g.dir == RIGHT);
    CHECK(g.score == 0);
    CHECK(g.steps_without_food == 0);
    CHECK(!g.game_over);
    CHECK(!(g.food == g.snake.front()));
}

static void test_place_food()
{
    SnakeGame g;
    g.snake.clear();
    for (int x = 0; x < GRID_WIDTH; ++x)
        for (int y = 0; y < GRID_HEIGHT; ++y)
            if (!(x == 3 && y == 4)) g.snake.push_back({x, y});
    g.place_food();
    CHECK(g.food == (Point{3, 4}));

    for (int i = 0; i < 100; ++i)
    {
        SnakeGame h;
        h.place_food();
        CHECK(h.food.x >= 0 && h.food.x < GRID_WIDTH);
        CHECK(h.food.y >= 0 && h.food.y < GRID_HEIGHT);
        CHECK(!(h.food == h.snake.front()));
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

    g.snake.clear();
    g.snake = {{5, 5}, {4, 5}, {3, 5}};
    CHECK(g.is_collision({4, 5}));
    // Tail will move away unless food is eaten.
    CHECK(!g.is_collision({3, 5}, false));
    CHECK(g.is_collision({3, 5}, true));
}

static void test_step_movement()
{
    SnakeGame g;
    g.food = {0, 0};
    Point start = g.snake.front();
    float r = g.step(STRAIGHT);
    CHECK(r == 0.0f);
    CHECK(g.snake.front() == (Point{start.x + 1, start.y}));
    CHECK(g.snake.size() == 1);
    CHECK(g.steps_without_food == 1);

    g.step(TURN_RIGHT);
    CHECK(g.dir == DOWN);
    g.step(TURN_LEFT);
    CHECK(g.dir == RIGHT);
    g.step(TURN_LEFT);
    CHECK(g.dir == UP);
    g.step(TURN_LEFT);
    CHECK(g.dir == LEFT);
}

static void test_step_food_and_death()
{
    SnakeGame g;
    g.food = {g.snake.front().x + 1, g.snake.front().y};
    float r = g.step(STRAIGHT);
    CHECK(r == 10.0f);
    CHECK(g.score == 1);
    CHECK(g.snake.size() == 2);
    CHECK(g.steps_without_food == 0);
    CHECK(!g.game_over);

    SnakeGame w;
    w.snake.clear();
    w.snake.push_back({GRID_WIDTH - 1, 0});
    w.dir = RIGHT;
    w.food = {0, 0};
    CHECK(w.step(STRAIGHT) == -10.0f);
    CHECK(w.game_over);

    SnakeGame s;
    s.food = {0, 0};
    s.steps_without_food = GRID_WIDTH * GRID_HEIGHT * 2 + 1;
    CHECK(s.step(STRAIGHT) == -10.0f);
    CHECK(s.game_over);
}

static void test_state_vector()
{
    SnakeGame g;
    g.snake.clear();
    g.snake.push_back({0, 0});
    g.dir = RIGHT;
    g.food = {5, 0};
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

    g.dir = LEFT;
    s = g.get_state_vector();
    CHECK(s[0] == 1.0f); // wall ahead
    CHECK(s[6] == 1.0f); // food behind

    g.dir = UP;
    g.food = {3, 3};
    s = g.get_state_vector();
    CHECK(s[4] == 1.0f); // food to the right (x>0)
    CHECK(s[6] == 1.0f); // and behind (y>0)

    g.dir = DOWN;
    s = g.get_state_vector();
    CHECK(s[3] == 1.0f);
    CHECK(s[5] == 1.0f); // dx>0 is left when facing down
}

int main()
{
    test_reset();
    test_place_food();
    test_is_collision();
    test_step_movement();
    test_step_food_and_death();
    test_state_vector();
    return TEST_RESULT();
}
