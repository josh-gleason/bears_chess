#include "movegen.hpp"
#include "bitboard.hpp"
#include "types.hpp"

namespace bears_chess {

bool is_check(const Board& board) {
    Bitboard bb_attacks = (
        board.side_to_move == Color::WHITE
        ? generate_attacks<Color::WHITE>(board, board.occupied)
        : generate_attacks<Color::BLACK>(board, board.occupied)
    );
    return nonzero(board.king_sq[idx(board.side_to_move)] & bb_attacks);
}

bool is_discovered_check(const Board& board, const Move& last_move) {
    if (is_castle(last_move.move_type))
        return false;
    
    Bitboard bb_checkers = (
        board.side_to_move == Color::WHITE
        ? generate_checkers<Color::WHITE>(board)
        : generate_checkers<Color::BLACK>(board)
    );
    return (popcount(bb_checkers) == 1) && nonzero(bb_checkers & ~bb_square(last_move.to));
}

template<Color color>
bool is_double_check(const Board& board) {
    return popcount(generate_checkers<color>(board)) == 2 && !is_checkmate<color>(board);
}

bool is_double_check(const Board& board) {
    if (board.side_to_move == Color::WHITE)
        return is_double_check<Color::WHITE>(board);
    return is_double_check<Color::BLACK>(board);
}

template<Color color>
bool is_checkmate(const Board& board) {
    BoardState<color, LegalPolicy> state(board);

    if (state.num_checkers > 0) {
        MoveList moves;

        generate_king_moves<color>(board, moves, state);

        if (state.num_checkers < 2) {
            if (!moves.empty()) return false;
            generate_castle_moves<color>(board, moves, state);
            if (!moves.empty()) return false;
            generate_knight_moves<color>(board, moves, state);
            if (!moves.empty()) return false;
            generate_slider_moves<color>(board, moves, state);
            if (!moves.empty()) return false;
            generate_pawn_moves<color>(board, moves, state);
        }

        return moves.empty();
    }

    return false;
}

bool is_checkmate(const Board& board) {
    if (board.side_to_move == Color::WHITE) {
        return is_checkmate<Color::WHITE>(board);
    }
    return is_checkmate<Color::BLACK>(board);

}

} // namespace bears_chess