#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

constexpr size_t MAX_MOVES = 218;

template <Color color>
MoveList generate_pseudo_legal_moves_(const Board& board) {
    MoveList moves;
    moves.reserve(MAX_MOVES);

    generate_knight_moves<color>(board, moves);
    generate_king_moves<color>(board, moves);
    generate_pawn_moves<color>(board, moves);
    generate_slider_moves<color, Piece::ROOK>(board, moves);
    generate_slider_moves<color, Piece::BISHOP>(board, moves);
    generate_castle_moves<color>(board, moves);

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
    BoardCache cache;
    MoveList moves;
    moves.reserve(MAX_MOVES);

    cache.opponent_attacks = calculate_opponent_attacks<color, true>(board);
    cache.checkers = calculate_checkers<color, true>(board, cache.opponent_attacks);
    int num_checkers = popcount(cache.checkers);

    if (num_checkers == 2) {
        generate_legal_king_moves<color>(board, moves, cache);
    } else {
        // hold mask of squares we can move pieces to to block check
        cache.block_mask = num_checkers == 0 ? Bitboard::FULL : cache.checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<color, Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<color, Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );

        generate_legal_king_moves<color>(board, moves, cache);
        generate_legal_knight_moves<color>(board, moves, cache);
        generate_legal_slider_moves<color, Piece::ROOK>(board, moves, cache);
        generate_legal_slider_moves<color, Piece::BISHOP>(board, moves, cache);
        generate_legal_pawn_moves<color>(board, moves, cache);
        generate_legal_castle_moves<color>(board, moves, cache);
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