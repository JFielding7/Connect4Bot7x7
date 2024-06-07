//
// Created by joe on 6/6/24.
//

#ifndef CONNECT4BOT7X7_ENGINE_H
#define CONNECT4BOT7X7_ENGINE_H

#include <unordered_map>

using namespace std;

#define i8 signed char
#define BEGINNING_GAME_DEPTH 18
#define SIZE ((1ul << 19) + 1)
#define MAX_TOTAL_MOVES 49
#define BEST_EVAl 22
#define WORST_EVAL -BEST_EVAl
#define DRAW 0
#define MOVE_ORDER (unsigned long) (3lu + (2 << 4) + (4 << 8) + (5 << 12) + (1 << 16) + (6 << 20))
#define START_HEIGHTS 0b00000001000000010000000100000001000000010000000100000001lu
#define ROWS 7
#define COLS 7
#define MAX_PLAYER_MOVES 25
#define IS_LEGAL 0b01111111011111110111111101111111011111110111111101111111lu
#define IS_UPPER_BOUND (1lu << 56)
#define BOUND_SHIFT 57
#define BOARD_MASK ((1lu << 56) - 1)
#define COLUMN_MASK 0b11111111lu
#define MOVE_MASK 0b1111lu
#define grid unsigned long
#define location unsigned long
#define mask unsigned long

typedef struct state_s {
    grid curr_pieces;
    grid opp_pieces;
    grid height_map;
    int moves_made;
} state;

state* encode(const char* board);

char* decode(grid curr_pieces, grid opp_pieces);

long evaluate_position(grid, grid, grid, int, long, long, unordered_map<grid, char>&, grid*, unsigned long*);

#endif //CONNECT4BOT7X7_ENGINE_H
