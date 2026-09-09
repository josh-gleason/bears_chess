#include "bindings.hpp"

#include <bears_chess/board_utils.hpp>

namespace nb = nanobind;

namespace bears_chess_py {

void bind_attrs(nb::module_& m) {
    m.attr("INITIAL_POSITION_FEN") = bears_chess::INITIAL_POSITION_FEN;
    m.attr("PERFT_POSITION_2_FEN") = bears_chess::PERFT_POSITION_2_FEN;
    m.attr("PERFT_POSITION_3_FEN") = bears_chess::PERFT_POSITION_3_FEN;
    m.attr("PERFT_POSITION_4_FEN") = bears_chess::PERFT_POSITION_4_FEN;
    m.attr("PERFT_POSITION_5_FEN") = bears_chess::PERFT_POSITION_5_FEN;
    m.attr("PERFT_POSITION_6_FEN") = bears_chess::PERFT_POSITION_6_FEN;
    m.attr("PERFT_FENS") = std::vector<const char*>({
        bears_chess::INITIAL_POSITION_FEN,
        bears_chess::PERFT_POSITION_2_FEN,
        bears_chess::PERFT_POSITION_3_FEN,
        bears_chess::PERFT_POSITION_4_FEN,
        bears_chess::PERFT_POSITION_5_FEN,
        bears_chess::PERFT_POSITION_6_FEN
    });
}

} // bears_chess_py