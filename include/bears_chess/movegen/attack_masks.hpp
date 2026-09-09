#pragma once
#include "bears_chess/types.hpp"
#include "bears_chess/bitboard.hpp"
#include "bears_chess/board.hpp"

namespace bears_chess {

template<Color color>
inline Bitboard generate_knight_attacks(const Board& board) {
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    Bitboard bb_knight_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_knights)) {
        bb_knight_attacks |= bb_attacks<Piece::KNIGHT>(from);
    }
    return bb_knight_attacks;
}

template<Color color, Piece move_type> requires is_bishop_or_rook<move_type>
inline Bitboard generate_slider_attacks(const Board& board, Bitboard bb_occupied) {
    Bitboard bb_sliders = (
        board.pieces[idx(color)][idx(move_type)] |
        board.pieces[idx(color)][idx(Piece::QUEEN)]
    );
    Bitboard bb_slider_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_sliders)) {
        bb_slider_attacks |= bb_attacks<move_type>(from, bb_occupied);
    }
    return bb_slider_attacks;
}

template<Color color>
inline Bitboard generate_pawn_attacks(const Board& board) {
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pawns)) {
        bb_pawn_attacks |= bb_attacks<color, Piece::PAWN>(from);
    }
    return bb_pawn_attacks;
}

template<Color color>
inline Bitboard generate_king_attacks(const Board& board) {
    return bb_attacks<Piece::KING>(board.king_sq[idx(color)]);
}

template <Color color>
inline Bitboard generate_attacks(const Board& board, Bitboard bb_occupied) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_attacks = (
        generate_knight_attacks<opponent_color>(board) |
        generate_slider_attacks<opponent_color, Piece::ROOK>(board, bb_occupied) |
        generate_slider_attacks<opponent_color, Piece::BISHOP>(board, bb_occupied) |
        generate_pawn_attacks<opponent_color>(board) |
        generate_king_attacks<opponent_color>(board)
    );

    return bb_attacks;
}

template<Color color>
inline Bitboard generate_checkers(const Board& board) {
    constexpr Color opponent_color = ~color;
    Square king_sq = board.king_sq[idx(color)];

    Bitboard bb_opponent_pawns = board.pieces[idx(opponent_color)][idx(Piece::PAWN)];
    Bitboard bb_opponent_knights = board.pieces[idx(opponent_color)][idx(Piece::KNIGHT)];
    Bitboard bb_opponent_queens = board.pieces[idx(opponent_color)][idx(Piece::QUEEN)];
    Bitboard bb_opponent_rooklike = (
        board.pieces[idx(opponent_color)][idx(Piece::ROOK)] | bb_opponent_queens
    );
    Bitboard bb_opponent_bishoplike = (
        board.pieces[idx(opponent_color)][idx(Piece::BISHOP)] | bb_opponent_queens
    );


    Bitboard occupied = board.occupied;
    Bitboard bb_knight_checkers = bb_opponent_knights & bb_attacks<Piece::KNIGHT>(king_sq);
    Bitboard bb_pawn_checkers = bb_opponent_pawns & bb_attacks<color, Piece::PAWN>(king_sq);
    Bitboard bb_rook_checkers = bb_opponent_rooklike & bb_attacks<Piece::ROOK>(king_sq, occupied);
    Bitboard bb_bishop_checkers = bb_opponent_bishoplike & bb_attacks<Piece::BISHOP>(king_sq, occupied);

    return bb_knight_checkers | bb_pawn_checkers | bb_rook_checkers | bb_bishop_checkers;
}

} // namespace bears_chess