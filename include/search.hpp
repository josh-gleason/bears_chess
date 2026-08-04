#pragma once

#include "search/repetition.hpp"
#include "search/transposition_table.hpp"
#include "search/search_options.hpp"

#include "types.hpp"
#include "board.hpp"
#include "movegen/movelist.hpp"
#include "score.hpp"

#include <chrono>
#include <atomic>
#include <functional>
#include <vector>
#include <span>

namespace bears_chess {

/**
- TODO finish implementing search_root and company
- TODO implement Transposition Table :
*/

class Search {
public:
    struct SearchResult {
        int16_t score;
        int depth;
        size_t nodes;
        std::vector<Move> principal_variation;
    };

    typedef std::function<void(const SearchResult&, std::chrono::milliseconds, int)> ReportCallback;

    Search(size_t tt_megabytes, ReportCallback on_report = {});

    void resize_tt(size_t megabytes);

    SearchResult go(const Board& root, std::span<const ZobristHash> hash_history, const SearchOptions& options);

    void stop();

    // call this after go returns
    void reset_stop();

    void register_report_callback(ReportCallback on_report_callback);

    void unregister_report_callback();

private:
    bool past_deadline();

    void report(const SearchResult& current_result);

    SearchResult search_root(const MoveList& move_list, int depth);

    int16_t mated_in_score(int16_t ply);

    bool prunable_tt_hit(const TTHit& hit, int depth, int16_t alpha, int16_t beta) const;

    template <Color side_to_move>
    int16_t negamax(int depth, int16_t ply, int16_t alpha=-SCORE_INF, int16_t beta=SCORE_INF);

    Board board{};
    std::atomic<bool> stop_requested{false};
    bool can_abort{false};
    bool has_aborted{false};
    RepetitionHashes repetition_hashes{};
    size_t nodes{};
    size_t max_node_count{MAX_NODE_LIMIT};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point deadline{MAX_DEADLINE};

    TranspositionTable transposition_table;
    ReportCallback on_report;
};

} // namespace bears_chess
