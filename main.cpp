#include <ctime>
#include <malloc.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <vector>
#include "engine.h"
#include "database_generator.h"

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

void load_database_into_caches(unordered_map<grid, i8>& lower_bound_cache, unordered_map<grid, i8>& upper_bound_cache) {
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
    for (auto pair : map0) {
        if (!map1.count(pair.first) || map1.at(pair.first) != pair.second) return false;
    }
    return true;
}

int main() {
    const char* board = "       \n"
                        "       \n"
                        "       \n"
                        "       \n"
                        "       \n"
                        "       \n"
                        " X     \0";
    state* game_state = encode(board);
    cout << "Depth: " << game_state->moves_made << "\n";

    int moves_made = game_state->moves_made;
    grid height_map = game_state->height_map;
    grid curr_pieces = game_state->curr_pieces;
    grid opp_pieces = game_state->opp_pieces;
    if (moves_made & 1) {
        curr_pieces = opp_pieces;
        opp_pieces = game_state->curr_pieces;
    }
    *game_state = state{.curr_pieces = curr_pieces, .opp_pieces = opp_pieces, .height_map = height_map, .moves_made = moves_made};

    long alpha = WORST_EVAL;
    long beta = BEST_EVAL;

    unordered_map<grid, i8> lower_bound_cache;
    unordered_map<grid, i8> upper_bound_cache;
    load_database_into_caches(lower_bound_cache, upper_bound_cache);

    grid* end_game_cache = (grid *) malloc(SIZE * sizeof(grid));
    for (int i = 0; i < SIZE; i++) end_game_cache[i] = 0;

    unsigned long pos = 0;
    clock_t begin = clock();
//    best_moves(game_state, lower_bound_cache, upper_bound_cache, end_game_cache, &pos);
//    generate_best_moves(game_state, 2, lower_bound_cache, upper_bound_cache, end_game_cache, &pos);
    long eval = evaluate_position(curr_pieces, opp_pieces, height_map, moves_made, alpha, beta, lower_bound_cache,upper_bound_cache, end_game_cache, &pos);
    cout << "Eval: " << eval << "\n";
//    for (auto position : next_states(game_state)) {
//        cout << decode(position.curr_pieces, position.opp_pieces) << "\n\n";
//        long eval = evaluate_position(position.curr_pieces, position.opp_pieces, position.height_map, position.moves_made,
//                                      alpha, beta, lower_bound_cache,upper_bound_cache, end_game_cache, &pos);
//        cout << "Eval: " << eval << "\n";
//    }
    clock_t end = clock();

    cout << "Pos: " << pos << "\n";
    cout << "Time: " << (double) (end - begin) / CLOCKS_PER_SEC << "\n";
    cout << "Lower bound cache size: " << lower_bound_cache.size() << "\n";
    cout << "Upper bound cache size: " << upper_bound_cache.size() << "\n";

//    write_caches_to_database(lower_bound_cache, upper_bound_cache);
//    unordered_map<grid, i8> l;
//    unordered_map<grid, i8> u;
//    load_database_into_caches(l, u);
//
//    cout << equals(lower_bound_cache, l) << "\n";
//    cout << equals(upper_bound_cache, u) << "\n";

    return 0;
}
