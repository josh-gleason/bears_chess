#pragma once

#include "board.hpp"

namespace bears_chess {

bool is_check(const Board& board);
bool is_discovered_check(const Board& board, const Move& last_move);
bool is_double_check(const Board& board);
bool is_checkmate(const Board& board);

} // namespace bears_chess
