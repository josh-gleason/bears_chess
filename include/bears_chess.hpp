#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/board.hpp"
#include "bears_chess/movegen.hpp"
#include "bears_chess/evaluation.hpp"
#include "bears_chess/search.hpp"
#include "bears_chess/perft_utils.hpp"
#include "bears_chess/board_utils.hpp"
#include "bears_chess/position_info.hpp"
#include "bears_chess/cli.hpp"
#include "bears_chess/log.hpp"
#include "bears_chess/zobrist.hpp"
#include "bears_chess/mcts.hpp"

namespace bears_chess {

inline void init() {
    init_bitboards();
    init_zobrist();
    log::init();
}

} // namespace bears_chess
