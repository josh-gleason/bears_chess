#include "types.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "perft_utils.hpp"
#include "board_utils.hpp"
#include "cli.hpp"
#include "log.hpp"

namespace bears_chess {

inline void init() {
    init_bitboards();
    log::init();
}

} // namespace bears_chess
