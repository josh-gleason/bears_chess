#include "search.hpp"

#include "movegen.hpp"
#include "evaluation.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <span>
#include <utility>
#include <queue>


namespace bears_chess {

constexpr int NODE_CHECK_INTERVAL_LOG2 = 10;
constexpr int NODE_CHECK_MASK = (1 << NODE_CHECK_INTERVAL_LOG2) - 1;

Search::Search(size_t tt_megabytes, ReportCallback on_report) :
    transposition_table(tt_megabytes),
    on_report(std::move(on_report))
{}

void Search::resize_tt(size_t megabytes) {
    transposition_table.resize(megabytes);
}

Search::SearchResult Search::go(
    std::stop_token stop_token,
    const Board& root,
    std::span<const ZobristHash> hash_history,
    const SearchOptions& options
) {
    start_time = std::chrono::steady_clock::now();

    stop_signal = stop_token;
    board = root;
    repetition_hashes.seed(hash_history, board.halfmove_clock);
    has_aborted = false;
    nodes = 0;
    transposition_table.new_search();

    MoveList initial_moves = generate_moves<LegalPolicy>(board);

    if (options.searchmoves.has_value()) {
        initial_moves = initial_moves.filter_moves(*options.searchmoves);
    }

    order_captures(initial_moves);

    deadline = options.deadline.value_or(MAX_DEADLINE);
    max_node_count = options.max_node_count.value_or(MAX_NODE_LIMIT);
    int max_depth = std::min(options.max_depth.value_or(MAX_DEPTH_LIMIT), MAX_DEPTH_LIMIT);

    num_pvs = options.num_pvs;

    SearchResult result{};
    if (initial_moves.empty()) {
        return result;
    }

    std::vector<RootMove> root_moves;
    root_moves.reserve(initial_moves.size());
    for (const auto& move : initial_moves) {
        root_moves.emplace_back(move);
    }

    for (int depth = 1; depth <= max_depth; ++depth) {
        // disallow abort in negamax to ensure a result is returned
        can_abort = (depth > 1);

        SearchResult iteration = search_root(root_moves, depth, options.num_pvs);

        if (depth > 1 && has_aborted) {
            break;
        }

        result = iteration;
        report(result);

        if (past_deadline()) {
            break;
        }
    }

    return result;
}

bool Search::past_deadline() const {
    return std::chrono::steady_clock::now() >= deadline;
}

void Search::report(const SearchResult& current_result) {
    if (on_report) {
        int hashfull = transposition_table.hashfull();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
        on_report(current_result, elapsed, hashfull);
    }
}

static inline int16_t capture_gain(const Board& board, const Move& move) {
    const Piece victim = (
        move.move_type == MoveType::EP_CAPTURE
        ? Piece::PAWN
        : board.get_piece_at<false>(move.to)    // Piece::NONE possible if non-captures
    );
    return PIECE_VALUES[idx(victim)] - PIECE_VALUES[idx(board.get_piece_at(move.from))];
}

void Search::order_captures(MoveList& moves) const {
    std::array<int16_t, MoveList::max_length> scores;
    std::transform(
        moves.begin(),
        moves.end(),
        scores.begin(),
        [this](const Move& move) { return capture_gain(board, move); }
    );

    std::ranges::sort(
        std::views::zip(moves, std::span(scores.begin(), scores.begin() + moves.size())),
        std::ranges::greater{},
        [](const auto& entry) {
            return std::get<1>(entry);
        }
    );
}

struct ScoredMove {
    Move move;
    int16_t score;
};

struct ScoredMoveGT {
    bool operator()(const ScoredMove& lhs, const ScoredMove& rhs) const {
        return lhs.score > rhs.score;
    }
};

Search::SearchResult Search::search_root(std::vector<RootMove>& ordered_moves, int depth, int num_pvs) {
    int16_t alpha = -SCORE_INF;
    int16_t beta = SCORE_INF;

    ++nodes;

    // ignores return value: continue to compute a move even if we are in a claimable draw
    repetition_hashes.record_and_check(0, board.halfmove_clock, board.hash);

    // track top moves with smallest (worst) at top
    std::priority_queue<int16_t, std::vector<int16_t>, std::greater<>> top_moves;

    for (size_t i = 0; i < ordered_moves.size(); ++i) {
        RootMove& move_info = ordered_moves[i];

        UndoInfo undo_info = board.do_move(move_info.move);
        int16_t score;
        bool is_pv = i < static_cast<size_t>(num_pvs);
        if (board.side_to_move == Color::WHITE) {
            score = -negamax<Color::WHITE>(depth - 1, 1, -beta, -alpha, is_pv);
        } else {
            score = -negamax<Color::BLACK>(depth - 1, 1, -beta, -alpha, is_pv);
        }
        board.undo_move(undo_info);

        if (has_aborted) {
            break;
        }

        move_info.score = score;
        move_info.pv_moves.clear();
        move_info.pv_moves.emplace_back(move_info.move);
        move_info.pv_moves.append(pv_record[0]);

        top_moves.emplace(score);
        if (top_moves.size() > static_cast<size_t>(num_pvs)) {
            top_moves.pop();
        }

        if (top_moves.size() == static_cast<size_t>(num_pvs)) {
            // alpha here represents lower bound on the num_pvs worst PV
            alpha = std::max(alpha, top_moves.top());
        }
    }

    std::vector<PrincipalVariation> result_pvs{};

    if (!has_aborted) {
        std::stable_sort(
            ordered_moves.begin(),
            ordered_moves.end(),
            [](const RootMove& lhs, const RootMove& rhs) { return lhs.score > rhs.score; }
        );

        size_t num_pv_results = std::min<size_t>(top_moves.size(), num_pvs);
        result_pvs.reserve(num_pv_results);
        for (size_t i = 0; i < num_pv_results; ++i) {
            result_pvs.emplace_back(ordered_moves[i].score, ordered_moves[i].pv_moves);
        }
    }

    return {
        depth,
        nodes,
        result_pvs
    };
}

int16_t Search::mated_in_score(int16_t ply) {
    return -SCORE_INF + 1 + ply;
}

bool Search::should_abort() const {
    return (
        can_abort
        && (nodes & NODE_CHECK_MASK) == 0
        && (stop_signal.stop_requested()
            || nodes >= max_node_count
            || past_deadline())
    );
}

template <Color side_to_move>
int16_t Search::negamax(int depth, int16_t ply, int16_t alpha, int16_t beta, bool is_pv) {
    ++nodes;

    // negamax doesn't start from the root node
    assert(ply > 0);

    PVMoveList& pv_row = pv_record[ply - 1];
    pv_row.clear();

    if (repetition_hashes.record_and_check(ply, board.halfmove_clock, board.hash)) {
        return DRAW_SCORE;
    }
    if (board.halfmove_clock >= 100) {
        return DRAW_SCORE;
    }

    if (should_abort()) {
        has_aborted = true;
        return 0;
    };

    Move best_move{Square::NONE, Square::NONE, MoveType::NONE};
    if (auto hit_result = transposition_table.probe(board.hash, ply)) {
        const auto& hit = *hit_result;
        // only return or narrow the window on non-pv lines
        if (!is_pv && hit.depth >= depth) {
            if (hit.bound == Bound::EXACT) {
                return hit.score;
            } else if (hit.bound == Bound::LOWER) {
                if (hit.score >= beta) {
                    return hit.score;
                }
                alpha = std::max(alpha, hit.score);
            } else {    // Bound::UPPER
                if (hit.score <= alpha) {
                    return hit.score;
                }
                beta = std::min(beta, hit.score);
            }
        }
        best_move = hit.best_move;
    }

    if (depth == 0) {
        return quiescence_search<side_to_move>(ply, alpha, beta);
    }

    // TODO build staged move picker to avoid full move generation, requires some refactor to movegen
    BoardState<side_to_move, LegalPolicy> state(board);
    auto moves = generate_moves<side_to_move>(board, state);
    order_captures(moves);

    if (best_move.move_type != MoveType::NONE) {
        moves.promote_to_front(best_move);
    }

    bool is_draw = moves.empty() && state.num_checkers == 0;
    int16_t max_score = is_draw ? DRAW_SCORE : mated_in_score(ply);

    // used for node bounds
    int16_t alpha_original = alpha;

    bool first = true;

    for (const auto& move : moves) {
        UndoInfo undo_info = board.do_move(move);
        int16_t score;
        score = -negamax<~side_to_move>(depth - 1, ply + 1, -beta, -alpha, is_pv && first);
        board.undo_move(undo_info);

        if (has_aborted) {
            return 0;
        }

        if (score > max_score) {
            max_score = score;
            best_move = move;
        }

        if (score > alpha) {
            alpha = score;
            pv_row.clear();
            pv_row.emplace_back(move);
            pv_row.append(pv_record[ply]);
        }

        if (alpha >= beta) {
            break;
        }

        first = false;
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

template<Color side_to_move>
int16_t Search::quiescence_search(int16_t ply, int16_t alpha, int16_t beta) {
    ++nodes;

    if (should_abort()) {
        has_aborted = true;
        return 0;
    }

    if (ply >= MAX_PLY - 1) {
        return evaluate<side_to_move>(board);
    }

    BoardState<side_to_move, LegalPolicy> state(board);
    const bool in_check = state.num_checkers > 0;

    int16_t max_score = -SCORE_INF;
    if (!in_check) {
        max_score = evaluate<side_to_move>(board);
        if (max_score >= beta) {
            return max_score;
        }
        alpha = std::max(alpha, max_score);
    }

    MoveList moves;
    if (in_check) {
        moves = generate_moves<side_to_move>(board, state);
        if (moves.empty()) {
            return mated_in_score(ply);
        }
    } else {
        moves = generate_moves<side_to_move, LegalPolicy, CaptureMoves>(board, state);
    }

    order_captures(moves);

    for (const auto& move : moves) {
        UndoInfo undo_info = board.do_move(move);
        int16_t score = -quiescence_search<~side_to_move>(ply + 1, -beta, -alpha);
        board.undo_move(undo_info);

        if (has_aborted) {
            return 0;
        }
        if (score > max_score) {
            max_score = score;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }

    return max_score;
}

} // namespace bears_chess
