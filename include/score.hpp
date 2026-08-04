#pragma once

#include <stdint.h>

namespace bears_chess {

constexpr int MAX_PLY = 256;
constexpr int16_t DRAW_SCORE = 0;
constexpr int16_t SCORE_INF = 0x7FFF;
constexpr int16_t MATE_SCORE_BOUND = SCORE_INF - MAX_PLY - 1;

constexpr int16_t PAWN_VALUE = 100;
constexpr int16_t KNIGHT_VALUE = 300;
constexpr int16_t BISHOP_VALUE = 300;
constexpr int16_t ROOK_VALUE = 500;
constexpr int16_t QUEEN_VALUE = 900;
constexpr int16_t KING_VALUE = 20000;

} // namespace bears_chess
