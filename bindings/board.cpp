#include "bindings.hpp"
#include "board.hpp"

namespace nb = nanobind;

namespace bears_chess_py {

void bind_board(nb::module_& m) {
    nb::class_<PyBoard>(m, "Board")
        .def(nb::init<const std::string&>(),
            nb::arg("fen") = std::string(INITIAL_POSITION_FEN))
        .def_prop_ro("fen", &PyBoard::fen)
        .def_prop_ro("side_to_move", &PyBoard::side_to_move)
        .def_prop_ro("halfmove_clock", &PyBoard::halfmove_clock)
        .def_prop_ro("hash", &PyBoard::hash)
        .def("legal_moves", &PyBoard::legal_moves)
        .def("do_move", &PyBoard::do_move, nb::arg("move"))
        .def("do_move_unchecked", &PyBoard::do_move_unchecked, nb::arg("move"))
        .def("undo_move", &PyBoard::undo_move)
        .def("is_check", &PyBoard::is_check)
        .def("is_checkmate", &PyBoard::is_checkmate)
        .def("is_stalemate", &PyBoard::is_stalemate)
        .def("is_legal_move", &PyBoard::is_legal_move, nb::arg("move"))
        .def("parse_move", &PyBoard::parse_move, nb::arg("move_str"));
}

} // bears_chess_py
