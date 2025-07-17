#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

MoveList generate_legal_moves(const Board& board) {
    BoardCache cache;
    MoveList moves;
    moves.reserve(218);

    cache.opponent_attacks = calculate_opponent_attacks<true>(board);
    Bitboard checkers = calculate_checkers<true>(board, cache.opponent_attacks);
    int num_checkers = popcount(checkers);

    if (num_checkers == 2) {
        generate_legal_king_moves(board, moves, cache);
    } else {
        // hold mask of squares we can move pieces to to block check
        cache.block_mask = num_checkers == 0 ? Bitboard::FULL : checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );

        generate_legal_king_moves(board, moves, cache);
        generate_legal_knight_moves(board, moves, cache);
        generate_legal_pawn_moves(board, moves, cache);
        generate_legal_slider_moves<Piece::ROOK>(board, moves, cache);
        generate_legal_slider_moves<Piece::BISHOP>(board, moves, cache);
        generate_legal_castle_moves(board, moves, cache);
    }

    return moves;
}

} // namespace bears_chess