#pragma once
#include "types.hpp"
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

struct CaptureMoves {
    static constexpr bool include_captures = true;
    static constexpr bool include_quiets = false;
};

struct QuietMoves {
    static constexpr bool include_captures = false;
    static constexpr bool include_quiets = true;
};

struct AllMoves {
    static constexpr bool include_captures = true;
    static constexpr bool include_quiets = true;
};

template<Color color, LegalityPolicy Policy, MoveSelection Selection=AllMoves>
inline MoveList generate_moves(const Board& board, const BoardState<color, Policy>& state) {
    MoveList moves;
    generate_king_moves<color, Policy, Selection>(board, moves, state);

    if constexpr (Policy::enforce_evasions) {
        if (state.num_checkers >= 2) {
            return moves;
        }
    }

    generate_knight_moves<color, Policy, Selection>(board, moves, state);
    generate_slider_moves<color, Policy, Selection>(board, moves, state);
    generate_pawn_moves<color, Policy, Selection>(board, moves, state);
    generate_castle_moves<color, Policy, Selection>(board, moves, state);

    return moves;
}


template<Color color, LegalityPolicy Policy, MoveSelection Selection=AllMoves>
inline MoveList generate_moves(const Board& board) {
    BoardState<color, Policy> state(board);
    return generate_moves<color, Policy, Selection>(board, state);
}


template <LegalityPolicy Policy, MoveSelection Selection=AllMoves>
MoveList generate_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE)
        return generate_moves<Color::WHITE, Policy, Selection>(board);
    return generate_moves<Color::BLACK, Policy, Selection>(board);
}

} // namespace bears_chess
