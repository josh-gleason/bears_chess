#pragma once

#include "search/repetition.hpp"
#include "search/transposition_table.hpp"
#include "search/search_options.hpp"

#include "types.hpp"
#include "board.hpp"
#include "movegen/movelist.hpp"
#include "score.hpp"

#include <chrono>
#include <functional>
#include <vector>
#include <span>
#include <stop_token>

namespace bears_chess {

using PVMoveList = BasicMoveList<MAX_DEPTH_LIMIT>;

struct PrincipalVariation {
    int16_t score;
    PVMoveList moves;
};

class Search {
public:
    struct SearchResult {
        int depth;
        size_t nodes;
        std::vector<PrincipalVariation> principal_variations;
    };

    typedef std::function<void(const SearchResult&, std::chrono::milliseconds, int)> ReportCallback;

    Search(size_t tt_megabytes, ReportCallback on_report = {});

    void resize_tt(size_t megabytes);

    SearchResult go(
        std::stop_token stop_token,
        const Board& root,
        std::span<const ZobristHash> hash_history,
        const SearchOptions& options
    );

private:
    struct RootMove {
        Move move;
        int16_t score{-SCORE_INF};
        PVMoveList pv_moves{};
    };

    struct NodeState {
        int16_t static_eval;
        Move current_move;
        std::array<Move, 2> killers;
        PVMoveList pv;
    };

    bool should_abort() const;
    bool past_deadline() const;

    void report(const SearchResult& current_result);

    SearchResult search_root(std::vector<RootMove>& ordered_moves, int depth, int num_pvs);

    template<Color child_side_to_move>
    inline int16_t search_child(int child_depth, int child_ply, int16_t alpha, int16_t beta, bool is_pv, bool full_window);

    int16_t mated_in_score(int ply);

    template <Color side_to_move>
    int16_t negamax(int depth, int ply, int16_t alpha=-SCORE_INF, int16_t beta=SCORE_INF, bool is_pv=false);

    template<Color side_to_move>
    int16_t quiescence_search(int ply, int16_t alpha, int16_t beta);

    Board board{};
    std::stop_token stop_signal;
    bool can_abort{false};
    bool has_aborted{false};
    RepetitionHashes repetition_hashes{};
    size_t nodes{};
    size_t max_node_count{MAX_NODE_LIMIT};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point deadline{MAX_DEADLINE};

    int num_pvs{1};
    std::array<NodeState, MAX_PLY> state_stack{};

    TranspositionTable transposition_table;
    ReportCallback on_report;
};

} // namespace bears_chess
