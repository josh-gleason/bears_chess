#pragma once

#include "types.hpp"
#include "board.hpp"
#include "movelist.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

    
template<Color color, MoveGenPolicy Policy>
MoveList generate_moves(const Board& board) {
    Policy policy;
    MoveList moves;

    calculate_king_unallowed<color>(board, policy);
    int num_checkers = calculate_checkers<color>(board, policy);

    if (num_checkers == 2) {
        generate_king_moves<color>(board, moves, policy);
    } else {
        calculate_pinned_pieces<color>(board, policy);
        generate_king_moves<color>(board, moves, policy);
        generate_knight_moves<color>(board, moves, policy);
        generate_slider_moves<color>(board, moves, policy);
        generate_pawn_moves<color>(board, moves, policy);
        generate_castle_moves<color>(board, moves, policy);
    }

    return moves;
}


template <MoveGenType T>
MoveList generate_moves(const Board& board) {
    if constexpr (T == MoveGenType::LEGAL) {
        if (board.side_to_move == Color::WHITE)
            return generate_moves<Color::WHITE, LegalPolicy>(board);
        return generate_moves<Color::BLACK, LegalPolicy>(board);
    } else {
        if (board.side_to_move == Color::WHITE)
            return generate_moves<Color::WHITE, PseudoLegalPolicy>(board);
        return generate_moves<Color::BLACK, PseudoLegalPolicy>(board);
    }
}

} // namespace bears_chess
