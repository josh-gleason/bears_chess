#include "bitboard.hpp"
#include "score.hpp"
#include "evaluation.hpp"

namespace bears_chess {

int16_t evaluate_white(const Board& board) {
    int16_t material = 0;

    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::PAWN)]) * PAWN_VALUE;
    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::KNIGHT)]) * KNIGHT_VALUE;
    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::BISHOP)]) * BISHOP_VALUE;
    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::ROOK)]) * ROOK_VALUE;
    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::QUEEN)]) * QUEEN_VALUE;
    material += popcount(board.pieces[idx(Color::WHITE)][idx(Piece::KING)]) * KING_VALUE;

    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::PAWN)]) * PAWN_VALUE;
    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::KNIGHT)]) * KNIGHT_VALUE;
    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::BISHOP)]) * BISHOP_VALUE;
    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::ROOK)]) * ROOK_VALUE;
    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::QUEEN)]) * QUEEN_VALUE;
    material -= popcount(board.pieces[idx(Color::BLACK)][idx(Piece::KING)]) * KING_VALUE;

    return material;
}

} // namespace bears_chess