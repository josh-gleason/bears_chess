#include "movegen.hpp"

namespace bears_chess {

struct BoardCache {

};

BoardCache generate_cache() {
    BoardCache cache;

    return cache;
}

inline void generate_knight_moves(const Board& board, const BoardCache& cache, MoveList& moves) {
    // find knights for our color
    Color color = board.side_to_move;

    Bitboard occupied = board.occupied_by_color[idx(color)];
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square square : bb_square_scan(bb_knights)) {
         
    }
}

MoveList generate_pseudo_legal_moves(const Board& board) {
    BoardCache cache = generate_cache();
    MoveList moves;

    generate_knight_moves(board, cache, moves);

    return moves;
}

} // namespace bears_chess
