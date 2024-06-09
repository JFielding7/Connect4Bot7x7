//
// Created by joe on 6/8/24.
//

#ifndef CONNECT4BOT7X7_DATABASE_GENERATOR_H
#define CONNECT4BOT7X7_DATABASE_GENERATOR_H

#include <vector>
#include "engine.h"

using namespace std;

void generate_best_moves(state*, int, unordered_map<grid, i8>&, unordered_map<grid, i8>&, grid*, unsigned long*);

vector<state> next_states(state* board);

#endif //CONNECT4BOT7X7_DATABASE_GENERATOR_H
