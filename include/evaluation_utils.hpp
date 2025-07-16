#pragma once

#include "bitboard.hpp"
#include "types.hpp"
#include "magics.hpp"
#include "board.hpp"

namespace bears_chess {

inline Bitboard generate_knight_attacks(const Board& board, Color color) {
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    Bitboard bb_knight_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_knights)) {
        bb_knight_attacks |= BB_KNIGHT_MOVES[idx(from)];
    }
    return bb_knight_attacks;
}

template<Piece move_type> requires is_bishop_or_rook<move_type>
inline Bitboard generate_slider_attacks(const Board& board, Color color, Bitboard bb_occupied) {
    Bitboard bb_pieces = (board.pieces[idx(color)][idx(move_type)] | board.pieces[idx(color)][idx(Piece::QUEEN)]);
    Bitboard bb_slider_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pieces)) {
        bb_slider_attacks |= magic_lookup<move_type>(from, bb_occupied);
    }
    return bb_slider_attacks;
}

inline Bitboard generate_pawn_attacks(const Board& board, Color color) {
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pawns)) {
        bb_pawn_attacks |= BB_CAPTURE_PAWN_MOVES[idx(color)][idx(from)];
    }
    return bb_pawn_attacks;
}

inline Bitboard generate_king_attacks(const Board& board, Color color) {
    return BB_KING_MOVES[idx(board.king_sq[idx(color)])];
}

template <bool omit_king=false>
inline Bitboard calculate_opponent_attacks(const Board& board) {
    Bitboard attacks = Bitboard::EMPTY;

    Color color = board.side_to_move;
    Color opponent_color = ~color;
    Bitboard occupied = board.occupied;
    
    if constexpr (omit_king) {
        occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
    }

    attacks |= generate_knight_attacks(board, opponent_color);
    attacks |= generate_slider_attacks<Piece::ROOK>(board, opponent_color, occupied);
    attacks |= generate_slider_attacks<Piece::BISHOP>(board, opponent_color, occupied);
    attacks |= generate_pawn_attacks(board, opponent_color);
    attacks |= generate_king_attacks(board, opponent_color);

    return attacks;
}

template<bool test_attacks=false>
inline Bitboard calculate_checkers(const Board& board, Bitboard opponent_attacks = Bitboard::EMPTY) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;
    Square king_square = board.king_sq[idx(color)];

    if constexpr (test_attacks) {
        // allow early exiting, valid either way
        Bitboard bb_king = bb_square(king_square);
        if (zero(opponent_attacks & bb_king)) {
            return Bitboard::EMPTY;
        }
    }

    Bitboard opponent_queens = board.pieces[idx(opponent_color)][idx(Piece::QUEEN)];
    Bitboard opponent_rooks = board.pieces[idx(opponent_color)][idx(Piece::ROOK)];
    Bitboard opponent_bishops = board.pieces[idx(opponent_color)][idx(Piece::BISHOP)];

    Bitboard knight_checkers = BB_KNIGHT_MOVES[idx(king_square)] & board.pieces[idx(opponent_color)][idx(Piece::KNIGHT)];
    Bitboard pawn_checkers = BB_CAPTURE_PAWN_MOVES[idx(color)][idx(king_square)] & board.pieces[idx(opponent_color)][idx(Piece::PAWN)];
    Bitboard rook_checkers = magic_lookup<Piece::ROOK>(king_square, board.occupied) & (opponent_rooks | opponent_queens);
    Bitboard bishop_checkers = magic_lookup<Piece::BISHOP>(king_square, board.occupied) & (opponent_bishops | opponent_queens);

    return knight_checkers | pawn_checkers | rook_checkers | bishop_checkers;
}

template<Piece move_type>
requires is_bishop_or_rook<move_type>
Bitboard calculate_pinned_pieces(const Board& board, Bitboard pin_masks[num_of<IndexDirection>], Bitboard& block_check) {
    Bitboard pinned = Bitboard::EMPTY;

    Square king_square = board.king_sq[idx(board.side_to_move)];
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Bitboard enemy_sliders = (
        board.pieces[idx(opponent_color)][idx(move_type)]
        | board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
    );

    for (auto pinner_sq : BBSquareScan(enemy_sliders)) {
        Bitboard between = BB_RAY<move_type>[idx(pinner_sq)][idx(king_square)];

        Bitboard my_pieces_between = between & board.occupied_by_color[idx(color)];
        Bitboard opponent_pieces_between = between & board.occupied_by_color[idx(opponent_color)];

        int my_piece_count = popcount(my_pieces_between);
        int opponent_piece_count = popcount(opponent_pieces_between);

        if (opponent_piece_count == 1) {
            if (my_piece_count == 0) {
                // king is checked by a slider
                block_check |= between;
            } else if (my_piece_count == 1) {
                IndexDirection pinner_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(pinner_sq)];
                pin_masks[idx(pinner_dir)] = between;
                pinned |= my_pieces_between;
            }
        }

    }

    return pinned;
}

inline bool is_check(const Board& board) {
    return nonzero(board.king_sq[idx(board.side_to_move)] & calculate_opponent_attacks(board));
}

inline bool is_discovered_check(const Board& board, const Move& last_move) {
    // a piece other than the last moved is checking the king
    return nonzero(calculate_checkers(board) ^ bb_square(last_move.to));
}

inline bool is_double_check(const Board& board) {
    return popcount(calculate_checkers(board)) == 2;
}

} // namespace bears_chess