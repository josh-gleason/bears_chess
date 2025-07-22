#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

template <Color color>
MoveList generate_pseudo_legal_moves_(const Board& board) {
    MoveList moves;

    PseudoLegalPolicy policy;

    generate_knight_moves<PseudoLegalPolicy, color>(board, moves, policy);
    generate_king_moves<PseudoLegalPolicy, color>(board, moves, policy);
    generate_pawn_moves<PseudoLegalPolicy, color>(board, moves, policy);
    generate_slider_moves<PseudoLegalPolicy, color, Piece::ROOK>(board, moves, policy);
    generate_slider_moves<PseudoLegalPolicy, color, Piece::BISHOP>(board, moves, policy);
    generate_castle_moves<PseudoLegalPolicy, color>(board, moves, policy);

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

    policy.cache.king_unallowed = calculate_opponent_attacks<color, true>(board);
    policy.cache.checkers = calculate_checkers<color, true>(board, policy.cache.king_unallowed);
    int num_checkers = popcount(policy.cache.checkers);

    if (num_checkers == 2) {
        generate_king_moves<LegalPolicy, color>(board, moves, policy);
    } else {
        policy.cache.block_mask = num_checkers == 0 ? Bitboard::FULL : policy.cache.checkers;
        policy.cache.pinned =
            calculate_pinned_pieces<color, Piece::ROOK>(board, policy.cache.pin_rays, policy.cache.block_mask)
            | calculate_pinned_pieces<color, Piece::BISHOP>(board, policy.cache.pin_rays, policy.cache.block_mask);

        generate_king_moves<LegalPolicy, color>(board, moves, policy);
        generate_knight_moves<LegalPolicy, color>(board, moves, policy);
        generate_slider_moves<LegalPolicy, color, Piece::ROOK>(board, moves, policy);
        generate_slider_moves<LegalPolicy, color, Piece::BISHOP>(board, moves, policy);
        generate_pawn_moves<LegalPolicy, color>(board, moves, policy);
        generate_castle_moves<LegalPolicy, color>(board, moves, policy);
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