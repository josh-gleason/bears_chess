#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>

namespace bears_chess_py {

void bind_search(nanobind::module_& m);
void bind_board(nanobind::module_& m);
void bind_move(nanobind::module_& m);
void bind_mcts(nanobind::module_& m);
void bind_encoding(nanobind::module_& m);

void bind_attrs(nanobind::module_& m);

}  // bears_chess_py
