//
// Created by joe on 6/8/24.
//
#include <iostream>
#include "database_generator.h"

void generate_best_moves(state* board, int depth, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos) {
    cout << decode(board->curr_pieces, board->opp_pieces) << "\nDepth: " << board->moves_made << "\n";
    vector<state> optimal_moves = best_moves(board, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
    if (depth == 1) return;
    for (auto optimal_move : optimal_moves) {
        for (auto move : next_states(&optimal_move)) {
            generate_best_moves(&move, depth - 1, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
        }
    }
}

vector<state> next_states(state* board) {
    grid curr_pieces = board->curr_pieces, opp_pieces = board->opp_pieces, height_map = board->height_map;
    int moves_made = board->moves_made + 1;

    vector<state> next_states;

    for (int i = 0; i < MOVE_ORDER_BIT_LENGTH; i += MOVE_BITS) {
        unsigned long col = (MOVE_ORDER >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));

        if (move & IS_LEGAL) {
            next_states.push_back(state{.curr_pieces = opp_pieces, .opp_pieces = curr_pieces | move, .height_map = height_map + move, .moves_made = moves_made});
        }
    }
    return next_states;
}
