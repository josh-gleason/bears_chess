#include "search.hpp"

#include "movegen.hpp"
#include "evaluation.hpp"
#include "position_info.hpp"

#include <algorithm>
#include <utility>

namespace bears_chess {

constexpr int NODE_CHECK_INTERVAL_LOG2 = 10;

Search::Search(size_t tt_megabytes, ReportCallback on_report) :
    transposition_table(tt_megabytes),
    on_report(std::move(on_report))
{}

void Search::resize_tt(size_t megabytes) {
    transposition_table.resize(megabytes);
}

Search::SearchResult Search::go(const Board& root, std::span<const ZobristHash> hash_history, const SearchOptions& options) {
    // expected to be called in thread, pass-by-value avoids dangling references and race conditions on engine state

    start_time = std::chrono::steady_clock::now();

    board = root;
    repetition_hashes.seed(hash_history, board.halfmove_clock);
    has_aborted = false;
    nodes = 0;
    transposition_table.new_search();

    MoveList root_moves = generate_moves<LegalPolicy>(board);

    if (options.searchmoves.has_value()) {
        root_moves = root_moves.filter_moves(*options.searchmoves);
    }
    deadline = options.deadline.value_or(MAX_DEADLINE);
    max_node_count = options.max_node_count.value_or(MAX_NODE_LIMIT);
    int max_depth = std::min(options.max_depth.value_or(MAX_DEPTH_LIMIT), MAX_DEPTH_LIMIT);

    SearchResult result{};
    if (root_moves.empty()) {
        return result;
    }

    for (int depth = 1; depth <= max_depth; ++depth) {
        // disallow abort in negamax to ensure a result is returned
        can_abort = (depth > 1);

        SearchResult iteration = search_root(root_moves, depth);

        if (depth > 1 && has_aborted) {
            break;
        }

        result = iteration;
        report(result);

        if (past_deadline()) {
            break;
        }

        if (!result.principal_variation.empty()) {
            root_moves.promote_to_front(result.principal_variation.front());
        }
    }

    return result;
}

void Search::stop() {
    stop_requested = true;
}

void Search::reset_stop() {
    stop_requested = false;
}

void Search::register_report_callback(ReportCallback on_report_callback) {
    on_report = std::move(on_report_callback);
}

void Search::unregister_report_callback() {
    on_report = {};
}

bool Search::past_deadline() {
    return std::chrono::steady_clock::now() >= deadline;
}

void Search::report(const SearchResult& current_result) {
    if (on_report) {
        int hashfull = transposition_table.hashfull();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
        on_report(current_result, elapsed, hashfull);
    }
}

Search::SearchResult Search::search_root(const MoveList& move_list, int depth) {
    Move best_move{Square::NONE, Square::NONE, MoveType::NONE};
    int16_t max_score = -SCORE_INF;

    int16_t alpha = -SCORE_INF;
    int16_t beta = SCORE_INF;

    ++nodes;

    // ignores return value: continue to compute a move even if we are in a claimable draw
    repetition_hashes.record_and_check(0, board.halfmove_clock, board.hash);

    for (const auto& move : move_list) {
        UndoInfo undo_info = board.do_move(move);
        int16_t score;
        if (board.side_to_move == Color::WHITE) {
            score = -negamax<Color::WHITE>(depth - 1, 1, -beta, -alpha);
        } else {
            score = -negamax<Color::BLACK>(depth - 1, 1, -beta, -alpha);
        }
        board.undo_move(undo_info);

        if (has_aborted) {
            break;
        }

        if (score > max_score) {
            max_score = score;
            best_move = move;
        }

        alpha = std::max(alpha, score);
    }

    return {
        max_score,
        depth,
        nodes,
        { best_move }
    };
}

int16_t Search::mated_in_score(int16_t ply) {
    return -SCORE_INF + 1 + ply;
}

bool Search::prunable_tt_hit(const TTHit& hit, int depth, int16_t alpha, int16_t beta) const {
    return (
        hit.depth >= depth && (
            hit.bound == Bound::EXACT ||
            (hit.bound == Bound::LOWER && hit.score >= beta) ||
            (hit.bound == Bound::UPPER && hit.score <= alpha))
    );
}

template <Color side_to_move>
int16_t Search::negamax(int depth, int16_t ply, int16_t alpha, int16_t beta) {
    ++nodes;

    if (repetition_hashes.record_and_check(ply, board.halfmove_clock, board.hash)) {
        return DRAW_SCORE;
    }
    if (board.halfmove_clock >= 100) {
        return DRAW_SCORE;
    }

    if (can_abort && (
            stop_requested
            || nodes >= max_node_count
            || ((nodes & ((1 << NODE_CHECK_INTERVAL_LOG2) - 1)) == 0 && past_deadline())
        ))
    {
        has_aborted = true;
        return 0;
    }

    Move best_move{Square::NONE, Square::NONE, MoveType::NONE};

    if (auto hit_result = transposition_table.probe(board.hash, ply)) {
        const auto& hit = *hit_result;
        if (prunable_tt_hit(hit, depth, alpha, beta)) {
            return hit.score;
        }
        best_move = hit.best_move;
    }

    if (depth == 0) {
        // TODO: quiescence search
        return evaluate<side_to_move>(board);
    }

    int16_t alpha_original = alpha;

    // TODO build staged move picker to avoid full move generation, requires some refactor to movegen
    MoveList moves = generate_moves<side_to_move, LegalPolicy>(board);

    if (best_move.move_type != MoveType::NONE) {
        moves.promote_to_front(best_move);
    }

    bool is_draw = moves.empty() && !is_check(board);
    int16_t max_score = is_draw ? DRAW_SCORE : mated_in_score(ply);

    // TODO: implement "pick_next_move" instead of ordering like this
    // TODO: record the top variation for this evaluation score
    // TODO: use principal variation to help move order
    for (const auto& move : moves) {
        UndoInfo undo_info = board.do_move(move);
        int16_t score = -negamax<~side_to_move>(depth - 1, ply + 1, -beta, -alpha);
        board.undo_move(undo_info);

        if (has_aborted) {
            return 0;
        }

        if (score > max_score) {
            max_score = score;
            best_move = move;
        }

        alpha = std::max(alpha, score);

        if (alpha >= beta) {
            break;
        }
    }

    Bound bound = Bound::EXACT;
    if (max_score <= alpha_original) {
        bound = Bound::UPPER;
    } else if (max_score >= beta) {
        bound = Bound::LOWER;
    }
    transposition_table.store(board.hash, best_move, max_score, depth, bound, ply);

    return max_score;
}

} // namespace bears_chess
