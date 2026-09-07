#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/function.h>

#include "bears_chess.hpp"

namespace nb = nanobind;

using namespace bears_chess;

class PyBoard {
public:
    PyBoard(const std::string& fen = INITIAL_POSITION_FEN) : board(load_fen(fen)) {
        hashes.push_back(board.hash);
    }

    std::string fen() const {
        return get_fen(board);
    }

    std::vector<Move> legal_moves() const {
        const MoveList moves = generate_moves<LegalPolicy>(board);
        return std::vector<Move>(moves.cbegin(), moves.cend());
    }

    void do_move(const Move& move) {
        if (!is_legal_move(move)) {
            throw std::invalid_argument("illegal move");
        }
        do_move_unchecked(move);
    }

    void do_move_unchecked(const Move& move) {
        undo.push_back(board.do_move(move));
        hashes.push_back(board.hash);
    }

    void undo_move() {
        if (undo.empty()) {
            throw std::runtime_error("undo_move() called at root position");
        }
        board.undo_move(undo.back());
        undo.pop_back();
        hashes.pop_back();
    }

    bool is_check() const {
        return bears_chess::is_check(board);
    }

    bool is_checkmate() const {
        return bears_chess::is_checkmate(board);
    }

    bool is_stalemate() const {
        return bears_chess::is_stalemate(board);
    }

    bool is_legal_move(const Move& move) const {
        if (board.side_to_move == Color::WHITE) {
            BoardState<Color::WHITE, LegalPolicy> state(board);
            return bears_chess::is_legal_move(board, state, move);
        } else {
            BoardState<Color::BLACK, LegalPolicy> state(board);
            return bears_chess::is_legal_move(board, state, move);
        }
    }

    int halfmove_clock() const {
        return board.halfmove_clock;
    }

    int64_t hash() const {
        return static_cast<uint64_t>(board.hash);
    }

    std::string side_to_move() const {
        return board.side_to_move == Color::WHITE ? "w" : "b";
    }

    const Board& c_board() const {
        return board;
    }

    std::span<const ZobristHash> history() const {
        return hashes;
    }

    Move parse_move(const std::string& move_str) const {
        return convert_uci_to_move(move_str, board);
    }

private:
    Board board;
    std::vector<UndoInfo> undo;
    std::vector<ZobristHash> hashes;
};

struct PyPrincipalVariation {
    int score;
    std::vector<Move> moves;
};

struct PySearchResult {
    PySearchResult() {}
    PySearchResult(const Search::SearchResult& r, int64_t elapsed_ms_, int hashfull_) {
        pvs.reserve(r.principal_variations.size());
        for (const PrincipalVariation& principal_variation : r.principal_variations) {            
            pvs.emplace_back(
                principal_variation.score,
                std::vector<Move>(principal_variation.moves.begin(), principal_variation.moves.end())
            );
        }
        depth = r.depth;
        nodes = r.nodes;
        elapsed_ms = elapsed_ms_;
        hashfull = hashfull_;
    }

    int depth;
    size_t nodes;
    std::vector<PyPrincipalVariation> pvs;
    int64_t elapsed_ms;
    int hashfull;
};

using ReportFn = std::function<void(const PySearchResult&)>;

static Search::ReportCallback make_adapter(std::optional<ReportFn> callback) {
    if (!callback) return {};
    return [callback = std::move(*callback)](
        const Search::SearchResult& r,
        std::chrono::milliseconds elapsed,
        int hashfull
    ) {
        nb::gil_scoped_acquire acquire;
        int64_t elapsed_ms = elapsed.count();
        try {
            callback(PySearchResult(r, elapsed_ms, hashfull));
        } catch (nb::python_error& e) {
            // don't kill the search
            e.discard_as_unraisable("search report callback");
        }
    };
}

class PySearch {
public:
    PySearch(
        size_t tt_megabytes,
        std::optional<ReportFn> on_report
    ) : search(tt_megabytes, make_adapter(std::move(on_report)))
    {}

    PySearchResult go(
        const PyBoard& board,
        std::optional<int> depth,
        std::optional<size_t> nodes,
        std::optional<int> movetime_ms
    ) {
        std::unique_lock lock(go_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            throw std::runtime_error("search already in progress");
        }

        std::stop_token token;
        {
            std::scoped_lock stop_lock(stop_mutex);
            stop_src = std::stop_source{};
            token = stop_src.get_token();
        }

        SearchOptions options;
        options.max_depth = depth;
        options.max_node_count = nodes;
        if (movetime_ms) {
            options.deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(*movetime_ms);
        }

        std::chrono::milliseconds elapsed;
        int hashfull;

        Search::SearchResult result;
        {
            nb::gil_scoped_release release;
            result = search.go(
                token,
                board.c_board(),
                board.history(),
                options
            );
            elapsed = search.elapsed();
            hashfull = search.hashfull();
        }

        return PySearchResult(std::move(result), elapsed.count(), hashfull);
    }

    void stop() {
        std::scoped_lock stop_lock(stop_mutex);
        stop_src.request_stop();
    }

private:
    Search search;
    std::mutex go_mutex;
    std::mutex stop_mutex;
    std::stop_source stop_src;
};

NB_MODULE(bears_chess, m) {
    init();

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
    
    nb::class_<PySearch>(m, "Search")
        .def(nb::init<size_t, std::optional<ReportFn>>(),
            nb::arg("tt_megabytes"),
            nb::arg("on_report") = nb::none())
        .def("go", &PySearch::go,
            nb::arg("board"),
            nb::arg("depth") = nb::none(),
            nb::arg("nodes") = nb::none(),
            nb::arg("movetime_ms") = nb::none()
        )
        .def("stop", &PySearch::stop);

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

    nb::class_<PyPrincipalVariation>(m, "PrincipalVariation")
        .def_ro("score", &PyPrincipalVariation::score)
        .def_ro("moves", &PyPrincipalVariation::moves);

    nb::class_<PySearchResult>(m, "SearchResult")
        .def_ro("depth", &PySearchResult::depth)
        .def_ro("nodes", &PySearchResult::nodes)
        .def_ro("pvs", &PySearchResult::pvs)
        .def_ro("hashfull", &PySearchResult::hashfull)
        .def_ro("elapsed_ms", &PySearchResult::elapsed_ms)
        .def_prop_ro("bestmove", [](const PySearchResult& r) -> std::optional<Move> {
            if (r.pvs.empty() || r.pvs.front().moves.empty()) {
                return std::nullopt;
            }
            return r.pvs.front().moves.front();
        })
        .def_prop_ro("score", [](const PySearchResult& r) -> std::optional<int> {
            if (r.pvs.empty()) {
                return std::nullopt;
            }
            return r.pvs.front().score;
        });

    m.attr("INITIAL_POSITION_FEN") = INITIAL_POSITION_FEN;
    m.attr("PERFT_POSITION_2_FEN") = PERFT_POSITION_2_FEN;
    m.attr("PERFT_POSITION_3_FEN") = PERFT_POSITION_3_FEN;
    m.attr("PERFT_POSITION_4_FEN") = PERFT_POSITION_4_FEN;
    m.attr("PERFT_POSITION_5_FEN") = PERFT_POSITION_5_FEN;
    m.attr("PERFT_POSITION_6_FEN") = PERFT_POSITION_6_FEN;
    m.attr("PERFT_FENS") = std::vector<const char*>({
        INITIAL_POSITION_FEN,
        PERFT_POSITION_2_FEN,
        PERFT_POSITION_3_FEN,
        PERFT_POSITION_4_FEN,
        PERFT_POSITION_5_FEN,
        PERFT_POSITION_6_FEN
    });
}