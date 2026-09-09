#include "bindings.hpp"

#include <bears_chess/board_utils.hpp>
#include <bears_chess/types.hpp>

#include <nanobind/operators.h>

#include <string>
#include <optional>

namespace nb = nanobind;

using namespace bears_chess;

namespace bears_chess_py {

void bind_move(nb::module_& m) {
    nb::class_<Move>(m, "Move")
        .def_prop_ro("from_sq", [](const Move& mv) { return static_cast<int>(idx(mv.from)); })
        .def_prop_ro("to_sq", [](const Move& mv) { return static_cast<int>(idx(mv.to)); })
        .def_prop_ro("uci", [](const Move& mv) { return convert_move_to_uci(mv); })
        .def_prop_ro("is_capture", [](const Move& mv) { return is_capture(mv.move_type); })
        .def_prop_ro("is_promotion", [](const Move& mv) { return is_promotion(mv.move_type); })
        .def_prop_ro("promotion", [](const Move& mv) -> std::optional<std::string> {
            if (!is_promotion(mv.move_type)) return std::nullopt;
            return piece_to_str(Color::BLACK, promote_to(mv.move_type));
        })
        .def(nb::self == nb::self)
        .def("__hash__", [](const Move& mv) { 
            return std::hash<Move>{}(mv);
        })
        .def("__repr__", [](const Move& mv) {
            return "Move('" + convert_move_to_uci(mv) + "')";
        });
}

} // bears_chess_py