//
// Created by joe on 6/6/24.
//

#include <malloc.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include "engine.h"


static long max(long arg0, long arg1) {
    return arg0 > arg1 ? arg0 : arg1;
}

static long min(long arg0, long arg1) {
    return arg0 < arg1 ? arg0 : arg1;
}

static grid reflect_state(grid state) {
    grid reflected = 0;
    for (int i = 0; i < BOARD_BITS; i+=COL_BITS) {
        reflected |= ((state >> i) & COLUMN_MASK) << (48 - i);
    }
    return reflected;
}

grid hash_code(state* board) {
    grid code = board->curr_pieces | board->height_map;
    grid reflected = reflect_state(code);
    if (reflected > code) return reflected;
    return code;
}

static int sort_by_threats(int threats) {
    int order = MOVE_ORDER;
    for (int i = 4; i < 28; i += 4) {
        int j = i, currThreats = (threats >> (order >> i & 0b1111) * 4 & 0b1111);
        while (j > 0 && currThreats > (threats >> (order >> (j - 4) & 0b1111) * 4 & 0b1111)) {
            j -= 4;
        }
        order = (order & (1 << j) - 1) + ((order >> i & 0b1111) << j) + ((order >> j & (1 << (i - j)) - 1) << (j + 4)) + (order >> (i + 4) << (i + 4));
    }
    return order;
}

static bool is_win(grid board) {
    for (int i = 1; i < 10; i += 1 / i * 5 + 1) {
        grid connections = board;
        for (int j = 0; j < 3; j++) connections = connections & (connections >> i);
        if (connections) return true;
    }
    return false;
}

static int count_threats(grid pieces, grid height_map) {
    int threatCount = 0;
    for (int col = 0; col < COLS; col++) {
        mask curr_col_mask = COLUMN_MASK << (col << 3);
        mask limit = curr_col_mask >> 1;
        for (location cell = height_map & curr_col_mask; cell < limit; cell <<= 1) {
            threatCount += is_win(pieces | cell);
        }
    }
    return threatCount;
}

// fix for negatives
static void update_alpha_beta_window(long* alpha, long* beta, grid entry) {
    if (entry & IS_UPPER_BOUND) *beta = min(*beta, (long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES);
    else *alpha = max(*alpha, (long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES);
}

mutex lower_bound_mutex;

void update_lower_bound(unordered_map<grid, i8>& lower_bound_cache, grid state_hash_code, long bound) {
    lower_bound_mutex.lock();
    if (!lower_bound_cache.count(state_hash_code) || bound > lower_bound_cache.at(state_hash_code)) {
        lower_bound_cache[state_hash_code] = (i8) bound;
    }
    lower_bound_mutex.unlock();
}

mutex upper_bound_mutex;

void update_upper_bound(unordered_map<grid, i8>& upper_bound_cache, grid state_hash_code, long bound) {
    upper_bound_mutex.lock();
    if (!upper_bound_cache.count(state_hash_code) || bound < upper_bound_cache.at(state_hash_code)) {
        upper_bound_cache[state_hash_code] = (i8) bound;
    }
    upper_bound_mutex.unlock();
}

long evaluate_position(grid curr_pieces, grid opp_pieces, grid height_map, int moves_made, long alpha, long beta, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos) {
    (*pos)++;

    if (moves_made == MAX_TOTAL_MOVES) return DRAW;
    alpha = max(alpha, -(MAX_PLAYER_MOVES - ((moves_made + 2) >> 1)));
    beta = min(beta, MAX_PLAYER_MOVES - ((moves_made + 1) >> 1));

    grid state_hash_code = curr_pieces | height_map;
    size_t index = state_hash_code % SIZE;

    if (moves_made > BEGINNING_GAME_DEPTH) {
        grid cache_entry = end_game_cache[index];
        if ((cache_entry & BOARD_MASK) == state_hash_code) update_alpha_beta_window(&alpha, &beta, cache_entry);
    }
    else {
        grid reflected = reflect_state(state_hash_code);
        if (reflected > state_hash_code) state_hash_code = reflected;
        if (lower_bound_cache.count(state_hash_code)) alpha = max(alpha, (long) lower_bound_cache.at(state_hash_code));
        if (upper_bound_cache.count(state_hash_code)) beta = min(beta, (long) upper_bound_cache.at(state_hash_code));
    }

    if (alpha >= beta) return alpha;

    int threats = 0;
    int forced_move_count = 0;
    grid forced_move = -1;
    for (int i = 0; i < MOVE_ORDER_BIT_LENGTH; i += MOVE_BITS) {
        unsigned long col = (MOVE_ORDER >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));

        if (move & IS_LEGAL) {
            grid updated_pieces = curr_pieces | move;
            if (is_win(updated_pieces)) return MAX_PLAYER_MOVES - (moves_made >> 1);

            // can't short-circuit because a different move may win the game
            if (is_win(opp_pieces | move)) {
                forced_move_count++;
                forced_move = move;
            }

            grid updated_height_map = height_map + move;
            grid next_state = opp_pieces | updated_height_map;

            if (moves_made < BEGINNING_GAME_DEPTH) {
                if (upper_bound_cache.count(next_state)) alpha = max(alpha, (long) -upper_bound_cache.at(next_state));
            }
            else {
                grid entry = end_game_cache[next_state % SIZE];
                if (((entry & BOARD_MASK) == next_state) && (entry & IS_UPPER_BOUND)) alpha = max(alpha, -((long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES));
            }

            if (alpha >= beta) return alpha;
            threats += count_threats(updated_pieces, updated_height_map) << (col << 2);
        }
    }

    if (forced_move_count > 1) return -(MAX_PLAYER_MOVES - ((moves_made >> 1) + 1));
    else if (forced_move_count) return -evaluate_position(opp_pieces, curr_pieces | forced_move, height_map + forced_move, moves_made + 1, -beta, -alpha, lower_bound_cache, upper_bound_cache, end_game_cache, pos);

    int order = sort_by_threats(threats);
    int moves_searched = 0;
    for (int i = 0; i < MOVE_ORDER_BIT_LENGTH; i += MOVE_BITS) {
        unsigned long col = (order >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));

        if (move & IS_LEGAL) {
            grid updated_pieces = curr_pieces | move;
            grid updated_height_map = height_map + move;

            long eval;
            if (moves_searched++ == 0) eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -beta, -alpha, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
            else {
                eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -alpha - 1, -alpha, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
                if (eval > alpha && eval < beta) eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -beta, -alpha, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
            }
            if (moves_made == 0) {
                puts(decode(updated_pieces, opp_pieces));
                cout << "Eval: " << eval << "\n";
            }
            alpha = max(alpha, eval);
            if (alpha >= beta) {
                if (moves_made > BEGINNING_GAME_DEPTH) end_game_cache[index] = state_hash_code | ((alpha + MAX_PLAYER_MOVES) << BOUND_SHIFT);
                else update_lower_bound(lower_bound_cache, state_hash_code, alpha);
                return alpha;
            }
        }
    }

    if (moves_made > BEGINNING_GAME_DEPTH) end_game_cache[index] = state_hash_code | IS_UPPER_BOUND | ((alpha + MAX_PLAYER_MOVES) << BOUND_SHIFT);
    else update_upper_bound(upper_bound_cache, state_hash_code, alpha);
    return alpha;
}

int depth(state* board) {
    grid pieces = board->curr_pieces | board->opp_pieces;
    int piece_count = 0;
    while (pieces) {
        piece_count++;
        pieces = pieces & (pieces - 1);
    }
    return piece_count;
}

vector<state> best_moves(state* board, unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache, grid* end_game_cache, unsigned long* pos) {
    unsigned long start = *pos;
    grid curr_pieces = board->curr_pieces, opp_pieces = board->opp_pieces, height_map = board->height_map;
//    cout << "\n" << decode(curr_pieces, opp_pieces) << "\n";
    int moves_made = board->moves_made + 1;

    long max_eval = WORST_EVAL;
    vector<state> optimal_moves;

    if (is_win(curr_pieces) || is_win(opp_pieces)) return optimal_moves;

    for (int i = 0; i < MOVE_ORDER_BIT_LENGTH; i += MOVE_BITS) {
        unsigned long col = (MOVE_ORDER >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));

        if (move & IS_LEGAL) {
            grid updated_pieces = curr_pieces | move;
            grid updated_height_map = height_map + move;

            auto next_state = state{.curr_pieces = opp_pieces, .opp_pieces = updated_pieces, .height_map = updated_height_map, .moves_made = moves_made};
	    if (is_win(updated_pieces)) {
		optimal_moves.clear();
		optimal_moves.push_back(next_state);
		return optimal_moves;
	    }
            long eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made, -max_eval, -max_eval + 1, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
            if (eval == max_eval) {
		eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made, WORST_EVAL, -max_eval, lower_bound_cache, upper_bound_cache, end_game_cache, pos);
		grid next_hash_code = opp_pieces | updated_height_map;
		grid reflected = reflect_state(next_hash_code);
                if (reflected > next_hash_code) next_hash_code = reflected;
	    	update_lower_bound(lower_bound_cache, next_hash_code, -eval);
		update_upper_bound(upper_bound_cache, next_hash_code, -eval);
	    }

            if (eval == max_eval) optimal_moves.push_back(next_state);
            else if (eval > max_eval) {
                max_eval = eval;
                optimal_moves.clear();
                optimal_moves.push_back(next_state);
            }
        }
    }
    return optimal_moves;
}

state* encode(const char* board) {
    auto* game_state = (state*) malloc(sizeof (state));
    game_state->curr_pieces = game_state->opp_pieces = game_state->height_map = game_state->moves_made = 0;
    for (int c = 0; c < COLS; c++) {
        location cell = 1lu << (c * (ROWS + 1));
        for (int r = 0; r < ROWS; r++) {
            char piece = board[(ROWS - 1 - r) * (COLS + 1) + c];
            if (piece == 'X') {
                game_state->curr_pieces |= cell;
                game_state->moves_made++;
            }
            else if (piece == 'O') {
                game_state->opp_pieces |= cell;
                game_state->moves_made++;
            }
            else {
                game_state->height_map |= cell;
                break;
            }
            cell <<= 1;
            if (r == (ROWS - 1)) {
                game_state->height_map |= cell;
                break;
            }
        }
    }
    return game_state;
}

char* decode(grid curr_pieces, grid opp_pieces) {
    char* board = (char *) malloc(ROWS * (COLS + 1));
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            location cell = (1lu << (r + c * (ROWS + 1)));
            size_t index = (ROWS - 1 - r) * (COLS + 1) + c;
            if (curr_pieces & cell) board[index] = 'X';
            else if (opp_pieces & cell) board[index] = 'O';
            else board[index] = ' ';
        }
        board[(ROWS - 1 - r) * (COLS + 1) + COLS] = r ? '\n' : '\0';
    }
    return board;
}
