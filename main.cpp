#include <ctime>
#include <malloc.h>
#include <iostream>
#include <unordered_map>
#include "engine.h"

using namespace std;

int main() {
//    unordered_map<grid, int> map;
//    for (grid i = 0; i < 3; i++) {
//        map[i] = (int) (i * 2 + 1 - 36);
//    }
//    cout << map.count(3);
//    for (auto pair : map) {
//        cout << pair.first << "\n";
//    }
//    return 0;
    const char* board = "   X   \n"
                        "   O   \n"
                        "   X   \n"
                        "   O   \n"
                        "   X   \n"
                        "   O   \n"
                        "   X   \0";
    state* game_state = encode(board);
    cout << "Depth: " << game_state->moves_made << "\n";

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
    grid* end_game_cache = (grid *) malloc(SIZE * sizeof(grid));
    for (int i = 0; i < SIZE; i++) {
        end_game_cache[i] = 0;
    }
    unordered_map<grid, char> beginning_game_cache;

    unsigned long pos = 0;
    clock_t begin = clock();
    long eval = evaluate_position(curr_pieces, opp_pieces, height_map, moves_made, alpha, beta, beginning_game_cache, end_game_cache, &pos);
    clock_t end = clock();

    cout << "Eval: " << eval << "\n";
    cout << "Pos: " << pos << "\n";
    cout << "Time: " << (double) (end - begin) / CLOCKS_PER_SEC << "\n";
    cout << "End game cache size: " << beginning_game_cache.size() << "\n";
    return 0;
}
