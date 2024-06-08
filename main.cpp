#include <ctime>
#include <malloc.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <vector>
#include "engine.h"

using namespace std;

void write_caches_to_database(unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache) {
    ofstream out;
    out.open("caches.bin", ios::binary | ios::out);

    grid* entries = (grid*) malloc((lower_bound_cache.size() + upper_bound_cache.size()) << 3);
    size_t i = 0;
    for (auto entry : lower_bound_cache) {
        entries[i++] = entry.first | (((long) entry.second + MAX_PLAYER_MOVES) << BOUND_SHIFT);
    }
    for (auto entry : upper_bound_cache) {
        entries[i++] = entry.first | IS_UPPER_BOUND | (((long) entry.second + MAX_PLAYER_MOVES) << BOUND_SHIFT);
    }

    out.write((char*) entries, (streamsize) (lower_bound_cache.size() + upper_bound_cache.size()) << 3);
    out.close();
}

void read_database_into_caches(unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache) {
        ifstream in("caches.bin", ios::binary | ios::ate);

        streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);

        grid* entries = (grid*) malloc(size);

        in.read((char*) entries, size);
        in.close();

        for (int i = 0; i < (size >> 3); i++) {
            grid entry = entries[i];
            i8 bound = (i8) ((long) (entry >> BOUND_SHIFT) - MAX_PLAYER_MOVES);
            if (entry & IS_UPPER_BOUND) upper_bound_cache[entry & BOARD_MASK] = bound;
            else lower_bound_cache[entry & BOARD_MASK] = bound;
        }
}

bool equals(unordered_map<grid, i8>& map0, unordered_map<grid, i8>& map1) {
    if (map0.size() != map1.size()) return false;
    cout << "same size ";
    for (auto pair : map0) {
        if (!map1.count(pair.first) || map1.at(pair.first) != pair.second) return false;
    }
    return true;
}

state* reflect_board(state* board) {
    auto reflected = (state*) malloc(sizeof(state));
    reflected->curr_pieces = reflected->opp_pieces = reflected->height_map = 0;
    reflected->moves_made = board->moves_made;

    for (int i = 0; i < BOARD_BITS; i+=COL_BITS) {
        reflected->curr_pieces |= ((board->curr_pieces >> i) & COLUMN_MASK) << (48 - i);
        reflected->opp_pieces |= ((board->opp_pieces >> i) & COLUMN_MASK) << (48 - i);
        reflected->height_map |= ((board->height_map >> i) & COLUMN_MASK) << (48 - i);
    }
    return reflected;
}

int main() {
    const char* board = "       \n"
                        "       \n"
                        "   O   \n"
                        "   X   \n"
                        "   OO  \n"
                        "   XX  \n"
                        "   XO  \0";
    state* game_state = encode(board);

    cout << decode(game_state->curr_pieces, game_state->opp_pieces) << "\n";
    cout << "Depth: " << game_state->moves_made << "\n";

    grid height_map = game_state->height_map;
    int moves_made = game_state->moves_made;
    grid curr_pieces = game_state->curr_pieces;
    grid opp_pieces = game_state->opp_pieces;
    if (moves_made & 1) {
        curr_pieces = opp_pieces;
        opp_pieces = game_state->curr_pieces;
    }
    *game_state = state{.curr_pieces = curr_pieces, .opp_pieces = opp_pieces, .height_map = height_map, .moves_made = moves_made};

    long alpha = WORST_EVAL;
    long beta = BEST_EVAL;
    grid* end_game_cache = (grid *) malloc(SIZE * sizeof(grid));
    for (int i = 0; i < SIZE; i++) {
        end_game_cache[i] = 0;
    }
    unordered_map<grid, i8> lower_bound_cache;
    unordered_map<grid, i8> upper_bound_cache;
//    read_database_into_caches(lower_bound_cache, upper_bound_cache);

    unsigned long pos = 0;
    clock_t begin = clock();
//    long eval = evaluate_position(curr_pieces, opp_pieces, height_map, moves_made, alpha, beta, lower_bound_cache, upper_bound_cache, end_game_cache, &pos);
    vector<state> optimal_moves = best_moves(game_state, lower_bound_cache, upper_bound_cache, end_game_cache, &pos);
    clock_t end = clock();

    for (auto move : optimal_moves) {
        cout << "\n";
        cout << decode(move.curr_pieces, move.opp_pieces) << "\n\n";
    }

//    cout << "Eval: " << eval << "\n";
    cout << "Pos: " << pos << "\n";
    cout << "Time: " << (double) (end - begin) / CLOCKS_PER_SEC << "\n";
    cout << "Lower cache size: " << lower_bound_cache.size() << "\n";
    cout << "Upper cache size: " << upper_bound_cache.size() << "\n";

//    write_caches_to_database(lower_bound_cache, upper_bound_cache);
//    unordered_map<grid, i8> l;
//    unordered_map<grid, i8> u;
//    read_database_into_caches(l, u);
//
//    cout << equals(lower_bound_cache, l) << "\n";
//    cout << equals(upper_bound_cache, u) << "\n";

    return 0;
}
