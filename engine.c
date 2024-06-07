//
// Created by joe on 6/6/24.
//

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>
#include "engine.h"

static long max(long arg0, long arg1) {
    return arg0 > arg1 ? arg0 : arg1;
}

static long min(long arg0, long arg1) {
    return arg0 < arg1 ? arg0 : arg1;
}

static grid reflect_board(grid board) {
    grid reflected = 0;
    for (int start = 0; start < 56; start+=8) {
        reflected |= ((board >> start) & COLUMN_MASK) << (48 - start);
    }
    return reflected;
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
    if (entry & IS_UPPER_BOUND) {
        *beta = min(*beta, (long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES);
    }
    else {
//        printf("%ld\n", (long) (entry >> BOUND_SHIFT));
        *alpha = max(*alpha, (long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES);
    }
}

long evaluate_position(grid curr_pieces, grid opp_pieces, grid height_map, int moves_made, long alpha, long beta, grid* cache, unsigned long* pos) {
    (*pos)++;

    if (moves_made == MAX_TOTAL_MOVES) return DRAW;
    alpha = max(alpha, -(MAX_PLAYER_MOVES - ((moves_made + 2) >> 1)));
    beta = min(beta, MAX_PLAYER_MOVES - ((moves_made + 1) >> 1));

    grid state = curr_pieces | height_map;
    size_t index = state % SIZE;
    grid cache_entry = cache[index];

    if ((cache_entry & BOARD_MASK) == state) {
        update_alpha_beta_window(&alpha, &beta, cache_entry);
    }
    if (alpha >= beta) return alpha;

    int threats = 0;
    int forced_move_count = 0;
    grid forced_move = -1;
    for (int i = 0; i < 28; i += 4) {
        unsigned long col = (MOVE_ORDER >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));
        if (move & IS_LEGAL) {
            grid updated_pieces = curr_pieces | move;
            if (is_win(updated_pieces)) {
                return MAX_PLAYER_MOVES - (moves_made >> 1);
            }

            // can't short-circuit because a different move may win the game
            if (is_win(opp_pieces | move)) {
                forced_move_count++;
                forced_move = move;
            }

            grid updated_height_map = height_map + move;

            grid next_state = opp_pieces | updated_height_map;
            size_t move_index = next_state % SIZE;
            grid entry = cache[move_index];
            if (((entry & BOARD_MASK) == next_state) && (entry & IS_UPPER_BOUND)) alpha = max(alpha, -((long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES));
            if (alpha >= beta) return alpha;

            threats += count_threats(updated_pieces, updated_height_map) << (col << 2);
        }
    }

    if (forced_move_count > 1) return -(MAX_PLAYER_MOVES - ((moves_made >> 1) + 1));
    else if (forced_move_count) return -evaluate_position(opp_pieces, curr_pieces | forced_move, height_map + forced_move, moves_made + 1, -beta, -alpha, cache, pos);

    int order = sort_by_threats(threats);
    int moves_searched = 0;
    for (int i = 0; i < 28; i += 4) {
        unsigned long col = (order >> i) & MOVE_MASK;
        location move = height_map & (COLUMN_MASK << (col << 3));
        if (move & IS_LEGAL) {
            grid updated_pieces = curr_pieces | move;
            grid updated_height_map = height_map + move;
            long eval;
            if (moves_searched++ == 0) eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -beta, -alpha, cache, pos);
            else {
                eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -alpha - 1, -alpha, cache, pos);
                if (eval > alpha && eval < beta) eval = -evaluate_position(opp_pieces, updated_pieces, updated_height_map, moves_made + 1, -beta, -alpha, cache, pos);
            }
            if (moves_made == 5) {
                puts(decode(updated_pieces, opp_pieces));
                printf("Eval: %ld", eval);
                puts("");
            }
            alpha = max(alpha, eval);
            if (alpha >= beta) {
                cache[index] = state | ((alpha + MAX_PLAYER_MOVES) << BOUND_SHIFT);
                return alpha;
            }
        }
    }
    cache[index] = state | IS_UPPER_BOUND | ((alpha + MAX_PLAYER_MOVES) << BOUND_SHIFT);
    return alpha;
}

state* encode(char* board) {
    state* game_state = (state*) malloc(sizeof (state));
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
