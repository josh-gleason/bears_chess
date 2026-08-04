#pragma once
#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/policy.hpp"
#include "movegen/attack_masks.hpp"

namespace bears_chess {

namespace detail {

template<bool enabled>
struct KingSafety {
    // enemy attacks as if our king were not present
    Bitboard king_unallowed;
};

template<>
struct KingSafety<false> {};

template<bool enabled>
struct Evasions {
    // mask of all pieces attacking the king
    Bitboard checkers;
    int num_checkers;
    // non-king moves are restricted to these squares to block checkers if present
    Bitboard evasion_mask;
};

template<>
struct Evasions<false> {};

template<bool enabled>
struct Pins {
    // all pieces that are pinned
    Bitboard pinned;
    // legal move mask for pinned piece, indexed by direction from king
    Bitboard pin_rays[num_of<IndexDirection>];

    Pins() : pinned{Bitboard::EMPTY} {}
};

template<>
struct Pins<false> {};

} // namespace detail

template<Color color, LegalityPolicy Policy>
struct BoardState :
    detail::KingSafety<Policy::enforce_king_safety>,
    detail::Evasions<Policy::enforce_evasions>,
    detail::Pins<Policy::enforce_pins>
{
    inline BoardState(const Board& board) {
        init_king_unallowed(board);
        init_evasions(board);
        init_pins(board);
    }

private:
    inline void init_king_unallowed(const Board& board) {
        if constexpr (Policy::enforce_king_safety) {
            Bitboard bb_occupied = board.occupied;
            bb_occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
            this->king_unallowed = generate_attacks<color>(board, bb_occupied);
        }
    }

    inline void init_evasions(const Board& board) {
        if constexpr (Policy::enforce_evasions) {
            constexpr Color opponent_color = ~color;

            Square king_sq = board.king_sq[idx(color)];

            if constexpr (Policy::enforce_king_safety) {
                // exit early if we are in check
                if (zero(this->king_unallowed & bb_square(king_sq))) {
                    this->checkers = Bitboard::EMPTY;
                    this->evasion_mask = Bitboard::FULL;
                    this->num_checkers = popcount(this->checkers);
                    return;
                }
            }

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

            this->checkers = bb_knight_checkers | bb_pawn_checkers | bb_rook_checkers | bb_bishop_checkers;
            this->num_checkers = popcount(this->checkers);

            switch (this->num_checkers) {
                case 1:
                    this->evasion_mask = this->checkers;
                    for (Square checker_sq : BBSquareScan(bb_rook_checkers | bb_bishop_checkers)) {
                        this->evasion_mask |= BB_RAY<Piece::QUEEN>[idx(checker_sq)][idx(king_sq)];
                    }
                    break;
                case 2:
                    this->evasion_mask = Bitboard::EMPTY;
                    break;
                default:
                    this->evasion_mask = Bitboard::FULL;
            }
        }
    }

    template<Piece move_type>
    inline void init_pins(const Board& board) requires is_bishop_or_rook<move_type>
    {
        constexpr Color opponent_color = ~color;

        Square king_square = board.king_sq[idx(color)];
        Bitboard bb_enemy_sliders = (
            board.pieces[idx(opponent_color)][idx(move_type)] |
            board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
        );

        for (Square pinner_sq : BBSquareScan(bb_enemy_sliders)) {
            Bitboard bb_between = BB_RAY<move_type>[idx(pinner_sq)][idx(king_square)];
            Bitboard bb_opponent_pieces_between = bb_between & board.occupied_by_color[idx(opponent_color)];
            int opponent_piece_count = popcount(bb_opponent_pieces_between);
            if (opponent_piece_count == 1) {
                Bitboard bb_my_pieces_between = bb_between & board.occupied_by_color[idx(color)];
                int my_piece_count = popcount(bb_my_pieces_between);
                if (my_piece_count == 1) {
                    IndexDirection pinner_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(pinner_sq)];
                    this->pin_rays[idx(pinner_dir)] = bb_between;
                    this->pinned |= bb_my_pieces_between;
                }
            }
        }
    }

    inline void init_pins(const Board& board) {
        if constexpr (Policy::enforce_pins) {
            if constexpr (Policy::enforce_evasions) {
                if (this->num_checkers >= 2) {
                    return;
                }
            }
            init_pins<Piece::ROOK>(board);
            init_pins<Piece::BISHOP>(board);
        }
    }
};

} // namespace bears_chess