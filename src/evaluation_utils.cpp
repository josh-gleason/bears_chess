#include "movegen_helpers.hpp"
#include "bitboard.hpp"
#include "types.hpp"

namespace bears_chess {

bool is_check(const Board& board) {
    Bitboard bb_attacks = (
        board.side_to_move == Color::WHITE
        ? calculate_attacks<Color::WHITE>(board, board.occupied)
        : calculate_attacks<Color::BLACK>(board, board.occupied)
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
    MoveList moves;

    int num_checkers = calculate_checkers<color>(board, policy);

    if (num_checkers == 2) {
        generate_king_moves<color>(board, moves, policy);
    } else if (num_checkers == 1) {
        calculate_pinned_pieces<color>(board, policy);
        generate_castle_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_king_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_knight_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_slider_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
        generate_pawn_moves<color>(board, moves, policy);
        if (!moves.empty()) return false;
    }

    return moves.empty();
}

bool is_checkmate(const Board& board) {
    if (board.side_to_move == Color::WHITE) {
        return is_checkmate_<Color::WHITE>(board);
    }
    return is_checkmate_<Color::BLACK>(board);

}

} // namespace bears_chess