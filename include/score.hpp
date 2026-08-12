#pragma once

#include "types.hpp"
#include <stdint.h>
#include <array>

namespace bears_chess {

constexpr int MAX_PLY = 256;
constexpr int16_t DRAW_SCORE = 0;
constexpr int16_t SCORE_INF = 0x7000;
constexpr int16_t MATE_SCORE_BOUND = SCORE_INF - MAX_PLY - 1;

constexpr int16_t PAWN_VALUE = 100;
constexpr int16_t KNIGHT_VALUE = 300;
constexpr int16_t BISHOP_VALUE = 300;
constexpr int16_t ROOK_VALUE = 500;
constexpr int16_t QUEEN_VALUE = 900;
constexpr int16_t KING_VALUE = 20000;

constexpr std::array<int16_t, num_of<Piece> + 1> PIECE_VALUES = {
    KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE, PAWN_VALUE, 0
};

} // namespace bears_chess
