#include "bindings.hpp"
#include "board.hpp"

#include <bears_chess/mcts.hpp>

#include <mutex>

namespace nb = nanobind;

using namespace bears_chess;

namespace bears_chess_py {

class PyUniformMCTS {
public:
    PyUniformMCTS(float c_puct, int max_ply, int batch_size) :
        mcts(UniformEvaluator{}, MCTSOptions{c_puct, max_ply, batch_size})
    {}

    MCTSResult go(const PyBoard& board, size_t simulations) {
        std::unique_lock lock(go_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            throw std::runtime_error("search already in progress");
        }

        std::stop_token token;
        {
            std::scoped_lock stop_lock(stop_mutex);
            stop_source = std::stop_source{};
            token = stop_source.get_token();
        }

        nb::gil_scoped_release release;
        return mcts.go(token, board.c_board(), board.hash_history(), simulations);
    }

    void stop() {
        std::scoped_lock stop_lock(stop_mutex);
        stop_source.request_stop();
    }

private:
    MCTS<UniformEvaluator> mcts;
    std::mutex go_mutex;
    std::mutex stop_mutex;
    std::stop_source stop_source;
};

void bind_mcts(nb::module_& m) {
    nb::class_<PyUniformMCTS>(m, "UniformMCTS")
        .def(nb::init<float, int, int>(),
            nb::arg("c_puct") = MCTSOptions{}.c_puct,
            nb::arg("max_ply") = MCTSOptions{}.max_ply,
            nb::arg("batch_size") = MCTSOptions{}.batch_size)
        .def("go", &PyUniformMCTS::go,
            nb::arg("board"),
            nb::arg("simulations"))
        .def("stop", &PyUniformMCTS::stop);

    nb::class_<MCTSRootStats>(m, "RootMoveStats")
          .def_ro("move", &MCTSRootStats::move)
          .def_ro("prior", &MCTSRootStats::prior)
          .def_ro("visits", &MCTSRootStats::visits)
          .def_ro("q", &MCTSRootStats::q);

    nb::class_<MCTSResult>(m, "MCTSResult")
          .def_ro("moves", &MCTSResult::moves)
          .def_ro("simulations", &MCTSResult::simulations)
          .def_ro("state_value", &MCTSResult::state_value)
          .def_prop_ro("best_move", &MCTSResult::best_move);
}

} // bears_chess_py