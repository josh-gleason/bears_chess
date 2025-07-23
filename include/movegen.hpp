#pragma once
#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/policy.hpp"
#include "movegen/init_policy.hpp"
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

    Bitboard king_unallowed;                    // enemy attacks as if our king were not present
    Bitboard checkers;                          // mask of all pieces attacking the king
    Bitboard evasion_mask;                      // non-king moves are restricted to these squares to block checkers if present
    Bitboard pinned;                            // all pieces that are pinned
    Bitboard pin_rays[num_of<IndexDirection>];  // legal move mask for pinned piece, indexed by direction from king
};


template<Color color, MoveGenPolicy Policy>
inline MoveList generate_moves(const Board& board) {
    Policy policy;
    MoveList moves;

    int num_checkers = init_policy<color>(board, policy);

    generate_king_moves<color>(board, moves, policy);

    if (num_checkers < 2) {
        generate_knight_moves<color>(board, moves, policy);
        generate_slider_moves<color>(board, moves, policy);
        generate_pawn_moves<color>(board, moves, policy);
        generate_castle_moves<color>(board, moves, policy);
    }

    return moves;
}

template <MoveGenPolicy Policy>
MoveList generate_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE)
        return generate_moves<Color::WHITE, Policy>(board);
    return generate_moves<Color::BLACK, Policy>(board);
}

} // namespace bears_chess
