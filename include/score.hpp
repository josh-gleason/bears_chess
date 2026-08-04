#pragma once

#include <stdint.h>

namespace bears_chess {

constexpr int MAX_PLY = 256;
constexpr int16_t DRAW_SCORE = 0;
constexpr int16_t SCORE_INF = 0x7FFF;
constexpr int16_t MATE_SCORE_BOUND = SCORE_INF - MAX_PLY - 1;

} // namespace bears_chess
