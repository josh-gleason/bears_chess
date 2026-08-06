#include "search.hpp"

#include "movegen.hpp"
#include "evaluation.hpp"

#include <algorithm>
#include <utility>

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

    MoveList root_moves = generate_moves<LegalPolicy>(board);

    if (options.searchmoves.has_value()) {
        root_moves = root_moves.filter_moves(*options.searchmoves);
    }

    order_captures(root_moves);

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
        : board.get_piece_at<false>(move.to)    // may be called with non-captures
    );
    return PIECE_VALUES[idx(victim)] - PIECE_VALUES[idx(board.get_piece_at(move.from))];
}

void Search::order_captures(MoveList& moves) const {
    // TODO: could be more efficient by computing capture_gain up front for all moves

    // order highest gain first
    std::sort(moves.begin(), moves.end(), 
        [this](const Move& a, const Move& b) {
            return capture_gain(board, a) > capture_gain(board, b);
        }
    );
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
int16_t Search::negamax(int depth, int16_t ply, int16_t alpha, int16_t beta) {
    ++nodes;

    if (repetition_hashes.record_and_check(ply, board.halfmove_clock, board.hash)) {
        return DRAW_SCORE;
    }
    if (board.halfmove_clock >= 100) {
        return DRAW_SCORE;
    }

    if (should_abort()) {
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
        return quiescence_search<side_to_move>(ply, alpha, beta);
    }

    int16_t alpha_original = alpha;

    // TODO build staged move picker to avoid full move generation, requires some refactor to movegen
    BoardState<side_to_move, LegalPolicy> state(board);
    auto moves = generate_moves<side_to_move>(board, state);
    order_captures(moves);

    if (best_move.move_type != MoveType::NONE) {
        moves.promote_to_front(best_move);
    }

    bool is_draw = moves.empty() && state.num_checkers == 0;
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
