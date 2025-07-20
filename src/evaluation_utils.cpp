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
    // a piece other than the last moved is checking the king
    // TODO fix this, giving wrong results during perft
    Bitboard bb_checkers = (
        board.side_to_move == Color::WHITE
        ? calculate_checkers<Color::WHITE>(board)
        : calculate_checkers<Color::BLACK>(board)
    );
    return nonzero(bb_checkers & ~bb_square(last_move.to));
}

bool is_double_check(const Board& board) {
    Bitboard bb_checkers = (
        board.side_to_move == Color::WHITE
        ? calculate_checkers<Color::WHITE>(board)
        : calculate_checkers<Color::BLACK>(board)
    );
    return popcount(bb_checkers) == 2;
}

template<Color color>
bool is_checkmate_(const Board& board) {
    MoveList moves;
    BoardCache cache;
    cache.opponent_attacks = calculate_opponent_attacks<color, true>(board);
    Bitboard checkers = calculate_checkers<color, true>(board, cache.opponent_attacks);
    int num_checkers = popcount(checkers);

    if (num_checkers == 2) {
        generate_legal_king_moves<color>(board, moves, cache);
        return moves.empty();
    } else if (num_checkers == 1) {
        generate_legal_king_moves<color>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_castle_moves<color>(board, moves, cache);
        if (!moves.empty()) return false;
        cache.block_mask = checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<color, Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );
        generate_legal_knight_moves<color>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_slider_moves<color, Piece::ROOK>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_slider_moves<color, Piece::BISHOP>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_pawn_moves<color>(board, moves, cache);
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