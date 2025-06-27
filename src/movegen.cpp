#include "movegen.hpp"

namespace bears_chess {

struct BoardCache {

};

BoardCache generate_cache() {
    BoardCache cache;

    return cache;
}

constexpr std::array<Bitboard, num_of<Square>> BB_KNIGHT_MOVES = []() {
    std::array<Bitboard, num_of<Square>> masks;
    for (Square sq : iter<Square>) {
        // TODO fix me
        Bitboard center = BB_SQUARE[idx(sq)];
        Bitboard mask = Bitboard::EMPTY;
        Rank r = rank_of(sq);
        File f = file_of(sq);
        bool n = r < Rank::_8;
        bool nn = r < Rank::_7;
        bool e = f < File::H;
        bool ee = f < File::G;
        bool s = r > Rank::_1;
        bool ss = r > Rank::_2;
        bool w = f > File::A;
        bool ww = f > File::B;
        if (n && ww) mask |= (center << (6));
        if (nn && w) mask |= (center << (15));
        if (n && ee) mask |= (center << (10));
        if (nn && e) mask |= (center << (17));
        if (s && ww) mask |= (center >> (10));
        if (ss && w) mask |= (center >> (17));
        if (s && ee) mask |= (center >> (6));
        if (ss && e) mask |= (center >> (15));
        masks[idx(sq)] = mask;
    }
    return masks;
}();

inline void generate_knight_moves(const Board& board, const BoardCache& cache, MoveList& moves) {
    // find knights for our color
    Color color = board.side_to_move;

    Bitboard occupied = board.occupied_by_color[idx(color)];
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square square : bb_scan(bb_knights)) {
         
    }
}

MoveList generate_pseudo_legal_moves(const Board& board) {
    BoardCache cache = generate_cache();
    MoveList moves;

    generate_knight_moves(board, cache, moves);

    return moves;
}

} // namespace bears_chess
