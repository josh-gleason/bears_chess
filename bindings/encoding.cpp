#include "bindings.hpp"
#include "board.hpp"

#include <bears_chess/mcts/encoding.hpp>

#include <nanobind/ndarray.h>

#include <algorithm>

namespace nb = nanobind;

using namespace bears_chess;

namespace bears_chess_py {

typedef nb::ndarray<nb::numpy, float, nb::shape<InputPlanes::COUNT, 8, 8>> PlanesArray;

static PlanesArray planes_to_numpy(const InputPlanes& planes) {
    float* data = new float[InputPlanes::SIZE];
    std::copy(planes.view().begin(), planes.view().end(), data);
    nb::capsule owner(
        data,
        [](void* p) noexcept { delete[] static_cast<float*>(p); }
    );
    return PlanesArray(data, {InputPlanes::COUNT, 8, 8}, owner);
}

void bind_encoding(nb::module_& m) {
    m.attr("POLICY_SIZE") = POLICY_SIZE;
    m.attr("INPUT_PLANES") = static_cast<size_t>(InputPlanes::COUNT);

    m.def("policy_index",
        [](const PyBoard& board, const Move& move) {
            return policy_index(move, board.c_board().side_to_move);
        },
        nb::arg("board"), nb::arg("move"));

    m.def("encode_board",
        [](const PyBoard& board) { return planes_to_numpy(encode_board(board.c_board())); },
        nb::arg("board"));
}

} // namespace bears_chess_py
