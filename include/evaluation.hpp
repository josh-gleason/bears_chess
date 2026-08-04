#pragma once

#include "board.hpp"
#include "score.hpp"

namespace bears_chess {

int16_t evaluate_white(const Board& board);

template <Color side_to_move=Color::NONE>
int16_t evaluate(const Board& board) {
    if constexpr (side_to_move == Color::WHITE) {
        return evaluate_white(board);
    } else if constexpr (side_to_move == Color::BLACK) {
        return -evaluate_white(board);
    } else {
        if (board.side_to_move == Color::WHITE) {
            return evaluate<Color::WHITE>(board);
        }
        return evaluate<Color::BLACK>(board);
    }
}

} // namespace bears_chess
