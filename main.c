#include <malloc.h>
#include <bits/types/clock_t.h>
#include <time.h>
#include "engine.h"

int main() {
    char* board =   "       \n"
                    "       \n"
                    "       \n"
                    "       \n"
                    "   O   \n"
                    "   XX  \n"
                    "   XO  \0";
//    1000000011000000010000000000000010000000100000001
//                            1000000000000000000000000
    state* game_state = encode(board);
    printf("%d\n", game_state->moves_made);

    grid height_map = game_state->height_map;
    int moves_made = game_state->moves_made;
    grid curr_pieces = game_state->curr_pieces;
    grid opp_pieces = game_state->opp_pieces;
    if (moves_made & 1) {
        curr_pieces = opp_pieces;
        opp_pieces = game_state->curr_pieces;
    }

    long alpha = WORST_EVAL;
    long beta = BEST_EVAl;
    grid* cache = (grid *) malloc(SIZE * sizeof(grid));
    for (int i = 0; i < SIZE; i++) {
        cache[i] = 0;
    }
    unsigned long pos = 0;

    clock_t begin = clock();

    long eval = evaluate_position(curr_pieces, opp_pieces, height_map, moves_made, alpha, beta, cache, &pos);

    clock_t end = clock();

    printf("Eval: %ld\n", eval);
    printf("Pos: %lu\n", pos);
    printf("Time: %lf\n", (double) (end - begin) / CLOCKS_PER_SEC);
    return 0;
}
