#pragma once
#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/policy.hpp"
#include "movegen/attack_masks.hpp"

namespace bears_chess {

template <Color color, MoveGenPolicy Policy> requires (!Policy::enforce_king_safety)
inline void init_king_unallowed(const Board& board, Policy& policy) {}

template <Color color, MoveGenPolicy Policy> requires Policy::enforce_king_safety
inline void init_king_unallowed(const Board& board, Policy& policy) {
    Bitboard bb_occupied = board.occupied;
    bb_occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
    policy.king_unallowed = generate_attacks<color>(board, bb_occupied);
}

template<Color color, MoveGenPolicy Policy> requires (!Policy::enforce_evasions)
inline int init_evasions(const Board& board, Policy& policy) {
    return 0;
}

template<Color color, MoveGenPolicy Policy> requires Policy::enforce_evasions
inline int init_evasions(const Board& board, Policy& policy) {
    constexpr Color opponent_color = ~color;

    Square king_sq = board.king_sq[idx(color)];

    if constexpr (Policy::enforce_king_safety) {
        // exit early if we are in check
        if (zero(policy.king_unallowed & bb_square(king_sq))) {
            policy.checkers = Bitboard::EMPTY;
            policy.evasion_mask = Bitboard::FULL;
            return 0;
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

    policy.checkers = bb_knight_checkers | bb_pawn_checkers | bb_rook_checkers | bb_bishop_checkers;
    int num_checkers = popcount(policy.checkers);

    switch (num_checkers) {
        case 1:
            policy.evasion_mask = policy.checkers;
            for (Square checker_sq : BBSquareScan(bb_rook_checkers | bb_bishop_checkers)) {
                policy.evasion_mask |= BB_RAY<Piece::QUEEN>[idx(checker_sq)][idx(king_sq)];
            }
            break;
        case 2:
            policy.evasion_mask = Bitboard::EMPTY;
            break;
        default:
            policy.evasion_mask = Bitboard::FULL;
    }

    return num_checkers;
}

template<Color color, Piece move_type, MoveGenPolicy Policy> 
    requires (is_bishop_or_rook<move_type> && Policy::enforce_pins)
inline void init_pins(const Board& board, Policy &policy) {
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
                policy.pin_rays[idx(pinner_dir)] = bb_between;
                policy.pinned |= bb_my_pieces_between;
            }
        }
    }
}

template<Color color, MoveGenPolicy Policy>
inline void init_pins(const Board& board, Policy &policy) {
    if constexpr (Policy::enforce_pins) {
        policy.pinned = Bitboard::EMPTY;
        init_pins<color, Piece::ROOK>(board, policy);
        init_pins<color, Piece::BISHOP>(board, policy);
    }
}


template<Color color, MoveGenPolicy Policy>
inline int init_policy(const Board& board, Policy& policy) {
    init_king_unallowed<color>(board, policy);
    int num_checkers = init_evasions<color>(board, policy);
    if (num_checkers < 2) {
        init_pins<color>(board, policy);
    }
    return num_checkers;
}

} // namespace bears_chess