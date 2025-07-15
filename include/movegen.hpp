#include "types.hpp"
#include "board.hpp"

namespace bears_chess {

MoveList generate_pseudo_legal_moves(const Board& board);
MoveList generate_legal_moves(Board& board);

} // namespace bears_chess
