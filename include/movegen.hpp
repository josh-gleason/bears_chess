#pragma once
#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/movelist.hpp"
#include "movegen/policy.hpp"
#include "movegen/board_state.hpp"
#include "movegen/king_moves.hpp"
#include "movegen/knight_moves.hpp"
#include "movegen/slider_moves.hpp"
#include "movegen/pawn_moves.hpp"

namespace bears_chess {

struct PseudoLegalPolicy {
    static constexpr bool enforce_king_safety = false;
    static constexpr bool enforce_evasions = false;
    static constexpr bool enforce_pins = false;
};

struct LegalPolicy {
    static constexpr bool enforce_king_safety = true;
    static constexpr bool enforce_evasions = true;
    static constexpr bool enforce_pins = true;
};

template<Color color, MoveGenPolicy Policy>
inline MoveList generate_moves(const Board& board) {
    MoveList moves;
    BoardState<color, Policy> state(board);

    generate_king_moves<color>(board, moves, state);

    if constexpr (Policy::enforce_evasions) {
        if (state.num_checkers >= 2) {
            return moves;
        }
    }

    generate_knight_moves<color>(board, moves, state);
    generate_slider_moves<color>(board, moves, state);
    generate_pawn_moves<color>(board, moves, state);
    generate_castle_moves<color>(board, moves, state);

    return moves;
}

template <MoveGenPolicy Policy>
MoveList generate_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE)
        return generate_moves<Color::WHITE, Policy>(board);
    return generate_moves<Color::BLACK, Policy>(board);
}

} // namespace bears_chess
