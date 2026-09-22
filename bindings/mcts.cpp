#include "bindings.hpp"
#include "board.hpp"

#include <bears_chess/mcts.hpp>
#ifdef BEARS_CHESS_HAS_TORCH
#include <bears_chess/mcts/torch_evaluator.hpp>
#endif

#include <mutex>

namespace nb = nanobind;

using namespace bears_chess;

namespace bears_chess_py {

template <Evaluator E>
class PyMCTS {
public:
    PyMCTS(E evaluator, MCTSOptions options) : mcts(std::move(evaluator), options) {}

    template <typename... Args>
    PyMCTS(MCTSOptions options, std::in_place_t, Args&&... args)
        : mcts(options, std::in_place, std::forward<Args>(args)...) {}

    template <typename F>
    auto run_locked(F&& f) {
        // lock out any calls to go until function is finished
        std::unique_lock lock(go_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            throw std::runtime_error("MCTS already in progress");
        }
        std::stop_token token;
        {
            std::scoped_lock stop_lock(stop_mutex);
            stop_source = std::stop_source{};
            token = stop_source.get_token();
        }
        nb::gil_scoped_release release;
        return f(mcts, token);
    }

    MCTSResult go(const PyBoard& board, size_t simulations) {
        return run_locked([&board, simulations](MCTS<E>& m, std::stop_token token) {
            return m.go(token, board.c_board(), board.hash_history(), simulations);
        });
    }

    void stop() {
        std::scoped_lock stop_lock(stop_mutex);
        stop_source.request_stop();
    }

private:
    MCTS<E> mcts;
    std::mutex go_mutex;
    std::mutex stop_mutex;
    std::stop_source stop_source;
};

using PyUniformMCTS = PyMCTS<UniformEvaluator>;
#ifdef BEARS_CHESS_HAS_TORCH
using PyTorchMCTS = PyMCTS<TorchEvaluator>;
#endif

class PySelfPlay {
public:
    PySelfPlay(
        size_t simulations,
        int temperature_plies,
        float temperature,
        int max_plies,
        uint64_t seed
    ) : self_play(SelfPlayOptions{simulations, temperature_plies, temperature, max_plies, seed})
    {}

    template <Evaluator E>
    SelfPlayGame play(PyMCTS<E>& mcts, const PyBoard& board) {
        return mcts.run_locked(
            [this, &board](MCTS<E>& m, std::stop_token token) {
                return self_play.play(m, board.c_board(), token);
            }
        );
    }

private:
    SelfPlay self_play;
};

static MCTSOptions make_options(
    float c_puct, int max_ply, int batch_size, float dirichlet_alpha, float dirichlet_epsilon, uint64_t seed
) {
    return MCTSOptions{c_puct, max_ply, batch_size, dirichlet_alpha, dirichlet_epsilon, seed};
}

void bind_mcts(nb::module_& m) {
    nb::class_<PyUniformMCTS>(m, "UniformMCTS")
        .def("__init__",
            [](PyUniformMCTS* self,
                float c_puct, int max_ply, int batch_size, float dirichlet_alpha, float dirichlet_epsilon,
                uint64_t seed
            ) {
                new (self) PyUniformMCTS(
                    make_options(c_puct, max_ply, batch_size, dirichlet_alpha, dirichlet_epsilon, seed),
                    std::in_place
                );
            },
            nb::arg("c_puct") = MCTSOptions{}.c_puct,
            nb::arg("max_ply") = MCTSOptions{}.max_ply,
            nb::arg("batch_size") = MCTSOptions{}.batch_size,
            nb::arg("dirichlet_alpha") = MCTSOptions{}.dirichlet_alpha,
            nb::arg("dirichlet_epsilon") = MCTSOptions{}.dirichlet_epsilon,
            nb::arg("seed") = MCTSOptions{}.seed)
        .def("go", &PyUniformMCTS::go, nb::arg("board"), nb::arg("simulations"))
        .def("stop", &PyUniformMCTS::stop);

#ifdef BEARS_CHESS_HAS_TORCH
    nb::class_<PyTorchMCTS>(m, "TorchMCTS")
        .def("__init__",
            [](PyTorchMCTS* self,
                const std::string& package_path, const std::string& device, int threads,
                float c_puct, int max_ply, int batch_size, float dirichlet_alpha, float dirichlet_epsilon,
                uint64_t seed
            ) {
                new (self) PyTorchMCTS(
                    make_options(c_puct, max_ply, batch_size, dirichlet_alpha, dirichlet_epsilon, seed),
                    std::in_place, package_path, device, threads
                );
            },
            nb::arg("package_path"),
            nb::arg("device") = "cpu",
            nb::arg("threads") = 1,
            nb::arg("c_puct") = MCTSOptions{}.c_puct,
            nb::arg("max_ply") = MCTSOptions{}.max_ply,
            nb::arg("batch_size") = MCTSOptions{}.batch_size,
            nb::arg("dirichlet_alpha") = MCTSOptions{}.dirichlet_alpha,
            nb::arg("dirichlet_epsilon") = MCTSOptions{}.dirichlet_epsilon,
            nb::arg("seed") = MCTSOptions{}.seed)
        .def("go", &PyTorchMCTS::go, nb::arg("board"), nb::arg("simulations"))
        .def("stop", &PyTorchMCTS::stop);
    m.attr("HAS_TORCH") = true;
#else
    m.attr("HAS_TORCH") = false;
#endif

    nb::class_<MCTSRootStats>(m, "RootMoveStats")
          .def_ro("move", &MCTSRootStats::move)
          .def_ro("prior", &MCTSRootStats::prior)
          .def_ro("visits", &MCTSRootStats::visits)
          .def_ro("q", &MCTSRootStats::q);

    nb::class_<MCTSResult>(m, "MCTSResult")
          .def_ro("moves", &MCTSResult::moves)
          .def_ro("simulations", &MCTSResult::simulations)
          .def_ro("state_value", &MCTSResult::state_value)
          .def_ro("eval_value", &MCTSResult::eval_value)
          .def_prop_ro("best_move", &MCTSResult::best_move);

    nb::enum_<GameTermination>(m, "GameTermination")
        .value("CHECKMATE", GameTermination::CHECKMATE)
        .value("STALEMATE", GameTermination::STALEMATE)
        .value("REPETITION", GameTermination::REPETITION)
        .value("FIFTY_MOVE", GameTermination::FIFTY_MOVE)
        .value("PLY_LIMIT", GameTermination::PLY_LIMIT)
        .value("STOPPED", GameTermination::STOPPED);

    nb::class_<SelfPlayPly>(m, "SelfPlayPly")
        .def_prop_ro("fen", [](const SelfPlayPly& p) { return get_fen(p.board); })
        .def_ro("stats", &SelfPlayPly::stats)
        .def_ro("selected_move", &SelfPlayPly::selected_move);

    nb::class_<SelfPlayGame>(m, "SelfPlayGame")
        .def_ro("plies", &SelfPlayGame::plies)
        .def_ro("result_white", &SelfPlayGame::result_white)
        .def_ro("termination", &SelfPlayGame::termination);

    nb::class_<PySelfPlay>(m, "SelfPlay")
        .def(nb::init<size_t, int, float, int, uint64_t>(),
            nb::arg("simulations") = SelfPlayOptions{}.simulations,
            nb::arg("temperature_plies") = SelfPlayOptions{}.temperature_plies,
            nb::arg("temperature") = SelfPlayOptions{}.temperature,
            nb::arg("max_plies") = SelfPlayOptions{}.max_plies,
            nb::arg("seed") = SelfPlayOptions{}.seed)
        .def("play", &PySelfPlay::play<UniformEvaluator>, nb::arg("mcts"), nb::arg("board"))
#ifdef BEARS_CHESS_HAS_TORCH
        .def("play", &PySelfPlay::play<TorchEvaluator>, nb::arg("mcts"), nb::arg("board"))
#endif
        ;
}

} // bears_chess_py