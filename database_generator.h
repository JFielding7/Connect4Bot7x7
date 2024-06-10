//
// Created by joe on 6/8/24.
//

#ifndef CONNECT4BOT7X7_DATABASE_GENERATOR_H
#define CONNECT4BOT7X7_DATABASE_GENERATOR_H

#include <vector>
#include "engine.h"

using namespace std;

void get_optimal_states(state* board, int depth, vector<state>& optimal_states, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos);

vector<state> next_states(state*);

void generate_best_moves(vector<state>&, size_t*, unordered_map<grid, i8>&, unordered_map<grid, i8>&, unsigned long*);

#endif //CONNECT4BOT7X7_DATABASE_GENERATOR_H
