#include "types.hpp"
#include "board.hpp"

namespace bears_chess {

MoveList generate_pseudo_legal_moves(const Board& board);
MoveList generate_legal_moves(const Board& board);

template <MoveGenType T>
MoveList generate_moves(const Board& board) {
    if constexpr (T == MoveGenType::LEGAL) {
        return generate_legal_moves(board);
    } else {
        return generate_pseudo_legal_moves(board);
    }
}

} // namespace bears_chess
