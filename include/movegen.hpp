#pragma once

#include "types.hpp"
#include "board.hpp"
#include "movelist.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

MoveList generate_pseudo_legal_moves(const Board& board);
MoveList generate_legal_moves(const Board& board);

template <MoveGenType T>
MoveList generate_moves(const Board& board) {
    if constexpr (T == MoveGenType::LEGAL) {
        return generate_legal_moves(board);
    } else {
        return generate_pseudo_legal_moves(board);
    }
}

template<Color color, MoveGenPolicy Policy>
MoveList generate_moves(const Board& board) {
    Policy policy;
    MoveList moves;

    int num_checkers;
    if constexpr (Policy::enforce_king_safety) {
        policy.king_unallowed = calculate_opponent_attacks<color, true>(board);
        policy.checkers = calculate_checkers<color>(board, policy.king_unallowed);
        num_checkers = popcount(policy.checkers);
        if (num_checkers == 2) {
            generate_king_moves<color>(board, moves, policy);
            return moves;
        }
    }

    if constexpr (Policy::enforce_evasions) {
        if constexpr (Policy::enforce_king_safety) {
            policy.evasion_mask = num_checkers == 0 ? Bitboard::FULL : policy.checkers;
        } else {
            Bitboard checkers = calculate_checkers<color>(board)
            policy.evasion_mask = nonzero(checkers) ? checkers : Bitboard::FULL;
        }
    }

    if constexpr (Policy::enforce_evasions && Policy::enforce_pins) {
        policy.pinned = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, policy.pin_rays, policy.evasion_mask) |
            calculate_pinned_pieces<color, Piece::BISHOP>(board, policy.pin_rays, policy.evasion_mask)
        );
    } else if constexpr (Policy::enforce_evasions) {
        // todo: compute evasion_mask without computing pins
    } else if constexpr (Policy::enforce_pins) {
        // todo: compute pins without enforcing evasion masks
        policy.pinned = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, policy.pin_rays) |
            calculate_pinned_pieces<color, Piece::BISHOP>(board, policy.pin_rays)
        );
    }


    policy.king_unallowed = calculate_opponent_attacks<color, true>(board);
    policy.checkers = calculate_checkers<color>(board, policy.king_unallowed);
    int num_checkers = popcount(policy.checkers);

    if (num_checkers == 2) {
        generate_king_moves<color>(board, moves, policy);
    } else {
        policy.evasion_mask = num_checkers == 0 ? Bitboard::FULL : policy.checkers;
        policy.pinned = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, policy.pin_rays, policy.evasion_mask) |
            calculate_pinned_pieces<color, Piece::BISHOP>(board, policy.pin_rays, policy.evasion_mask)
        );

        generate_king_moves<color>(board, moves, policy);
        generate_knight_moves<color>(board, moves, policy);
        generate_slider_moves<color, Piece::ROOK>(board, moves, policy);
        generate_slider_moves<color, Piece::BISHOP>(board, moves, policy);
        generate_pawn_moves<color>(board, moves, policy);
        generate_castle_moves<color>(board, moves, policy);
    }

    return moves;
}

template<MoveGenPolicy Policy>
MoveList generate_moves(const Board& board) {
    if (board.side_To_move == Color::WHITE) {
        return generate_moves<Color::WHITE, Policy>(board);
    } else {
        return generate_moves<Color::BLACK, Policy>(board);
    }
}

} // namespace bears_chess
