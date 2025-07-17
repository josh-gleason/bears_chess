#include "movegen_helpers.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include "magics.hpp"

namespace bears_chess {

bool is_check(const Board& board) {
    return nonzero(board.king_sq[idx(board.side_to_move)] & calculate_opponent_attacks(board));
}

bool is_discovered_check(const Board& board, const Move& last_move) {
    // a piece other than the last moved is checking the king
    return nonzero(calculate_checkers(board) & ~bb_square(last_move.to));
}

bool is_double_check(const Board& board) {
    return popcount(calculate_checkers(board)) == 2;
}

bool is_checkmate(const Board& board) {
    MoveList moves;
    BoardCache cache;
    cache.opponent_attacks = calculate_opponent_attacks<true>(board);
    Bitboard checkers = calculate_checkers<true>(board, cache.opponent_attacks);
    int num_checkers = popcount(checkers);

    if (num_checkers == 2) {
        generate_legal_king_moves(board, moves, cache);
        return moves.empty();
    } else if (num_checkers == 1) {
        generate_legal_king_moves(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_castle_moves(board, moves, cache);
        if (!moves.empty()) return false;
        cache.block_mask = checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );
        generate_legal_knight_moves(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_slider_moves<Piece::ROOK>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_slider_moves<Piece::BISHOP>(board, moves, cache);
        if (!moves.empty()) return false;
        generate_legal_pawn_moves(board, moves, cache);
        return moves.empty();
    }
    return false;
}

} // namespace bears_chess