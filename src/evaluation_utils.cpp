#include "movegen_helpers.hpp"
#include "bitboard.hpp"
#include "types.hpp"

namespace bears_chess {

bool is_check(const Board& board) {
    Bitboard bb_attacks = (
        board.side_to_move == Color::WHITE
        ? calculate_opponent_attacks<Color::WHITE>(board)
        : calculate_opponent_attacks<Color::BLACK>(board)
    );
    return nonzero(board.king_sq[idx(board.side_to_move)] & bb_attacks);
}

bool is_discovered_check(const Board& board, const Move& last_move) {
    if (is_castle(last_move.move_type))
        return false;
    Bitboard bb_checkers = (
        board.side_to_move == Color::WHITE
        ? calculate_checkers<Color::WHITE>(board)
        : calculate_checkers<Color::BLACK>(board)
    );
    return (popcount(bb_checkers) == 1) && nonzero(bb_checkers & ~bb_square(last_move.to));
}

template<Color color>
bool is_double_check_(const Board& board) {
    return popcount(calculate_checkers<color>(board)) == 2 && !is_checkmate_<color>(board);
}

bool is_double_check(const Board& board) {
    return (board.side_to_move == Color::WHITE ? is_double_check_<Color::WHITE>(board) : is_double_check_<Color::BLACK>(board));
}

template<Color color>
bool is_checkmate_(const Board& board) {
    LegalPolicy policy;

    policy.king_unallowed = calculate_opponent_attacks<color, true>(board);
    policy.checkers = calculate_checkers<color>(board, policy.king_unallowed);
    int num_checkers = popcount(policy.checkers);

    MoveList moves;

    if (num_checkers == 2) {
        generate_king_moves<color>(board, moves, policy);
        return moves.empty();
    } else if (num_checkers == 1) {
        generate_king_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_castle_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        policy.evasion_mask = num_checkers == 0 ? Bitboard::FULL : policy.checkers;
        policy.pinned = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, policy.pin_rays, policy.evasion_mask) |
            calculate_pinned_pieces<color, Piece::BISHOP>(board, policy.pin_rays, policy.evasion_mask)
        );
        generate_knight_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_slider_moves<color, Piece::ROOK>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_slider_moves<color, Piece::BISHOP>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_pawn_moves<color>(board, moves, policy);
        return moves.empty();
    }
    return false;
}

bool is_checkmate(const Board& board) {
    if (board.side_to_move == Color::WHITE) {
        return is_checkmate_<Color::WHITE>(board);
    }
    return is_checkmate_<Color::BLACK>(board);

}

} // namespace bears_chess