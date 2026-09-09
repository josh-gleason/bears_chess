#include "search.hpp"

#include "movegen.hpp"
#include "evaluation.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <span>
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

static MoveList generate_initial_moves(const Board& board) {
    MoveList moves;
    if (board.side_to_move == Color::WHITE) {
        BoardState<Color::WHITE, LegalPolicy> board_state(board);
        MovePicker<Color::WHITE> picker(board, board_state);
        Move move;
        while (!(move = picker.next()).is_none()) {
            moves.emplace_back(move);
        }
    } else {
        BoardState<Color::BLACK, LegalPolicy> board_state(board);
        MovePicker<Color::BLACK> picker(board, board_state);
        Move move;
        while (!(move = picker.next()).is_none()) {
            moves.emplace_back(move);
        }
    }
    return moves;
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
    state_stack.fill({0, MOVE_NONE, {MOVE_NONE, MOVE_NONE}, {}});
    history = {};

    MoveList initial_moves = generate_initial_moves(board);

    if (options.searchmoves.has_value()) {
        initial_moves = initial_moves.filter_moves(*options.searchmoves);
    }

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
        on_report(current_result, elapsed(), hashfull());
    }
}

Search::SearchResult Search::search_root(std::vector<RootMove>& ordered_moves, int depth, int num_pvs) {
    ++nodes;

    // ignores return value: continue to compute a move even if we are in a claimable draw
    repetition_hashes.record_and_check(0, board.halfmove_clock, board.hash);

    size_t num_pvs_actual = std::min<size_t>(ordered_moves.size(), num_pvs);

    for (size_t pv_index = 0; pv_index < num_pvs_actual; ++pv_index) {
        int16_t alpha = -SCORE_INF;
        int16_t beta = SCORE_INF;

        size_t best_move_index = pv_index;
        int16_t best_score = -SCORE_INF;

        for (size_t move_index = pv_index; move_index < ordered_moves.size(); ++move_index) {
            RootMove& move_info = ordered_moves[move_index];

            bool first = move_index == pv_index;

            UndoInfo undo_info = board.do_move(move_info.move);
            int16_t score;
            if (board.side_to_move == Color::WHITE) {
                score = search_child<Color::WHITE>(depth - 1, 1, alpha, beta, true, first);
            } else {
                score = search_child<Color::BLACK>(depth - 1, 1, alpha, beta, true, first);
            }
            board.undo_move(undo_info);

            if (has_aborted) {
                break;
            }

            move_info.score = score;
            move_info.pv_moves.clear();
            move_info.pv_moves.emplace_back(move_info.move);
            move_info.pv_moves.append(state_stack[1].pv);

            if (score > best_score) {
                best_score = score;
                best_move_index = move_index;
            }

            alpha = std::max(alpha, score);
        }
        if (has_aborted) {
            break;
        }

        // elevate the pv to the appropriate place, avoids using it next iteration
        std::swap(ordered_moves[pv_index], ordered_moves[best_move_index]);
    }

    std::vector<PrincipalVariation> result_pvs{};

    if (!has_aborted) {
        std::stable_sort(
            ordered_moves.begin(),
            ordered_moves.end(),
            [](const RootMove& lhs, const RootMove& rhs) { return lhs.score > rhs.score; }
        );

        result_pvs.reserve(num_pvs_actual);
        for (size_t i = 0; i < num_pvs_actual; ++i) {
            result_pvs.emplace_back(ordered_moves[i].score, ordered_moves[i].pv_moves);
        }
    }

    return {
        depth,
        nodes,
        result_pvs
    };
}

template <Color child_side_to_move>
inline int16_t Search::search_child(int child_depth, int child_ply, int16_t alpha, int16_t beta, bool is_pv, bool full_window) {
    
    if (full_window) {
        return -negamax<child_side_to_move>(child_depth, child_ply, -beta, -alpha, is_pv);
    }

    // null window aiming for score <= alpha implying line is no better than best
    int16_t score = -negamax<child_side_to_move>(child_depth, child_ply, -(alpha + 1), -alpha, false);

    // null window failed, need to search proper
    // score < beta included to avoid searching when beta cutoff is imminent
    if (!has_aborted && score > alpha && score < beta) {
        score = -negamax<child_side_to_move>(child_depth, child_ply, -beta, -alpha, is_pv);
    }

    return score;
}

int16_t Search::mated_in_score(int ply) {
    return -SCORE_INF + 1 + static_cast<int16_t>(ply);
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

inline void Search::update_history(Color side_to_move, Square from, Square to, int depth) {
    int bonus = depth * depth;
    int16_t& h = history[idx(side_to_move)][idx(from)][idx(to)];
    h += bonus - h * bonus / MAX_HISTORY_SCORE;
}

template <Color side_to_move>
int16_t Search::negamax(int depth, int ply, int16_t alpha, int16_t beta, bool is_pv) {
    ++nodes;

    // negamax doesn't start from the root node
    assert(ply > 0);
    assert(ply < MAX_PLY);

    NodeState& node_state = state_stack[ply];
    PVMoveList& pv_row = node_state.pv;

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

    Move tt_move = MOVE_NONE;
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
        tt_move = hit.best_move;
    }

    if (depth == 0) {
        return quiescence_search<side_to_move>(ply, alpha, beta);
    }

    BoardState<side_to_move, LegalPolicy> state(board);
    MovePicker move_picker(
        board,
        state,
        tt_move,
        node_state.killers,
        true,
        history[idx(side_to_move)]
    );

    int16_t max_score = mated_in_score(ply);
    Move best_move = MOVE_NONE;

    // used for node bounds
    int16_t alpha_original = alpha;

    int moves_searched = 0;

    Move move;
    while (!(move = move_picker.next()).is_none()) {
        ++moves_searched;

        UndoInfo undo_info = board.do_move(move);
        int16_t score = search_child<~side_to_move>(depth - 1, ply + 1, alpha, beta, is_pv, moves_searched == 1);
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
            pv_row.append(state_stack[ply + 1].pv);
        }

        if (alpha >= beta) {
            if (!is_capture(move.move_type) && !is_promotion(move.move_type)) {
                update_history(side_to_move, move.from, move.to, depth);
                if (move != state_stack[ply].killers[0]) {
                    state_stack[ply].killers[1] = state_stack[ply].killers[0];
                    state_stack[ply].killers[0] = move;
                }
            }
            break;
        }
    }

    bool is_draw = moves_searched == 0 && state.num_checkers == 0;
    if (is_draw) {
        max_score = DRAW_SCORE;
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
int16_t Search::quiescence_search(int ply, int16_t alpha, int16_t beta) {
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

    bool include_quiets = in_check;
    MovePicker move_picker(
        board,
        state,
        MOVE_NONE,
        {MOVE_NONE, MOVE_NONE},
        include_quiets,
        history[idx(side_to_move)]
    );

    Move move;
    while (!(move = move_picker.next()).is_none()) {
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

    if (in_check && max_score == -SCORE_INF) {
        return mated_in_score(ply);
    }

    return max_score;
}

    
int Search::hashfull() const {
    return transposition_table.hashfull();
}

std::chrono::milliseconds Search::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
}


} // namespace bears_chess
