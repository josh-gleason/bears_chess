#include "bindings.hpp"
#include "board.hpp"

#include <bears_chess/search.hpp>

#include <mutex>

namespace nb = nanobind;

using namespace bears_chess;

namespace bears_chess_py {

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

void bind_search(nb::module_& m) {
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
}

} // bears_chess_py