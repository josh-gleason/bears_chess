#pragma once

#include "bitboard.hpp"
#include "types.hpp"
#include "magics.hpp"
#include "board.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

inline bool is_check(const Board& board) {
    return nonzero(board.king_sq[idx(board.side_to_move)] & calculate_opponent_attacks(board));
}

inline bool is_discovered_check(const Board& board, const Move& last_move) {
    // a piece other than the last moved is checking the king
    return nonzero(calculate_checkers(board) ^ bb_square(last_move.to));
}

inline bool is_double_check(const Board& board) {
    return popcount(calculate_checkers(board)) == 2;
}

} // namespace bears_chess