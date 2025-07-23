#pragma once
#include "types.hpp"
#include "movelist.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/policy.hpp"

namespace bears_chess {

template<Color color, MoveGenPolicy Policy>
inline void generate_knight_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_quiet = ~board.occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];
    if constexpr (Policy::enforce_evasions) {
        bb_quiet &= policy.evasion_mask;
        bb_capture &= policy.evasion_mask;
    }

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    if constexpr (Policy::enforce_pins) {
        bb_knights &= ~policy.pinned;
    }

    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = bb_attacks<Piece::KNIGHT>(from);
        moves.append_bb(from, bb_moves & bb_quiet, MoveType::QUIET);
        moves.append_bb(from, bb_moves & bb_capture, MoveType::CAPTURE);
    }
}

} // namespace bears_chess
