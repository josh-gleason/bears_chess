#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

template <Color color>
MoveList generate_pseudo_legal_moves_(const Board& board) {
    MoveList moves;

    PseudoLegalPolicy policy;

    generate_king_moves<color>(board, moves, policy);
    generate_knight_moves<color>(board, moves, policy);
    generate_pawn_moves<color>(board, moves, policy);
    generate_slider_moves<color, Piece::ROOK>(board, moves, policy);
    generate_slider_moves<color, Piece::BISHOP>(board, moves, policy);
    generate_castle_moves<color>(board, moves, policy);

    return moves;
}

MoveList generate_pseudo_legal_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE) {
        return generate_pseudo_legal_moves_<Color::WHITE>(board);
    }
    return generate_pseudo_legal_moves_<Color::BLACK>(board);
}

template<Color color>
MoveList generate_legal_moves_(const Board& board) {
    LegalPolicy policy;
    MoveList moves;

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

MoveList generate_legal_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE) {
        return generate_legal_moves_<Color::WHITE>(board);
    }
    return generate_legal_moves_<Color::BLACK>(board);
}

} // namespace bears_chess