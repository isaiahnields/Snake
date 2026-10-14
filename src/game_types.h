#pragma once

const int GRID_WIDTH = 30;
const int GRID_HEIGHT = 30;
const int CELL_SIZE = 30;
const int WINDOW_WIDTH = GRID_WIDTH * CELL_SIZE;
const int WINDOW_HEIGHT = GRID_HEIGHT * CELL_SIZE;
// Smallest window that still shows the full board, its borders and the game HUD legibly.
const int MIN_WINDOW_WIDTH = 600;
const int MIN_WINDOW_HEIGHT = 600;

struct Point
{
    int x, y;
    bool operator==(const Point &other) const
    {
        return x == other.x && y == other.y;
    }
};

enum Direction
{
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};
enum Action
{
    STRAIGHT = 0,
    TURN_RIGHT = 1,
    TURN_LEFT = 2
};
