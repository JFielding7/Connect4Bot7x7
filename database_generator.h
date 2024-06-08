//
// Created by joe on 6/8/24.
//

#ifndef CONNECT4BOT7X7_DATABASE_GENERATOR_H
#define CONNECT4BOT7X7_DATABASE_GENERATOR_H

#include <vector>
#include "engine.h"

using namespace std;

void generate_best_moves(state* board, int depth);

vector<state> next_states(state* board);

#endif //CONNECT4BOT7X7_DATABASE_GENERATOR_H
