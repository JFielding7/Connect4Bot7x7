//
// Created by joe on 6/8/24.
//
#include <iostream>
#include <unordered_set>
#include <mutex>
#include "database_generator.h"
#include "engine.h"

void generate_optimal_states(state* board, int depth, vector<state>& optimal_states, unordered_set<grid>& seen, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos) {
    grid code = hash_code(board);
    if (seen.count(code)) return;
    seen.insert(code);

    if (depth == 0) {
        optimal_states.push_back(*board);
        return;
    }

    vector<state> optimal_moves = best_moves(board, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
    for (auto optimal_move : optimal_moves) {
        for (auto move : next_states(&optimal_move)) {
            generate_optimal_states(&move, depth - 1, optimal_states, seen, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
        }
    }
}

void get_optimal_states(state* board, int depth, vector<state>& optimal_states, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos) {
    unordered_set<grid> seen;
    generate_optimal_states(board, depth, optimal_states, seen, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
}

vector<state> next_states(state* board) {
    grid curr_pieces = board->curr_pieces, opp_pieces = board->opp_pieces, height_map = board->height_map;
    int moves_made = board->moves_made + 1;

    vector<state> next_states;
    unsigned long ORDER = (6lu << 4) + (1lu << 8) + (5lu << 12) + (2lu << 16) + (4lu << 20) + (3lu << 24);
    cout << ORDER << "\n";
    for (int i = 0; i < MOVE_ORDER_BIT_LENGTH; i += MOVE_BITS) {
        unsigned long col = (MOVE_ORDER >> i) & MOVE_MASK;
	cout << col << " ";
        location move = height_map & (COLUMN_MASK << (col << 3));

        if (move & IS_LEGAL) {
            next_states.push_back(state{.curr_pieces = opp_pieces, .opp_pieces = curr_pieces | move, .height_map = height_map + move, .moves_made = moves_made});
        }
    }
    return next_states;
}

mutex exclusion;

state* get_next_position(vector<state>& positions, size_t* i) {
    exclusion.lock();
    state* position = nullptr;
    if (*i != positions.size()) position = &positions.at((*i)++);
    exclusion.unlock();
    return position;
}

void generate_best_moves(vector<state>& positions, size_t* i, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, unsigned long* pos) {
    grid* end_game_cache = (grid *) malloc(SIZE * sizeof(grid));
    for (int j = 0; j < SIZE; j++) end_game_cache[j] = 0;

    state* position = get_next_position(positions, i);
    while (position != nullptr) {
        cout << "Count: " + to_string(*i) + "\n";
        best_moves(position, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
        position = get_next_position(positions, i);
    }
}
