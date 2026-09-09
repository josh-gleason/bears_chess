#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/movegen.hpp"
#include "bears_chess/evaluation.hpp"

namespace bears_chess {

using HistoryTable = std::array<std::array<int16_t, num_of<Square>>, num_of<Square>>;

inline constexpr HistoryTable EMPTY_HISTORY{};

constexpr int16_t MAX_HISTORY_SCORE = 8000;
constexpr int16_t KILLER_0_BONUS = 16000;
constexpr int16_t KILLER_1_BONUS = 15999;
constexpr int16_t UNDERPROMOTION_PENALTY = -8001;

static_assert(KILLER_1_BONUS > MAX_HISTORY_SCORE);
static_assert(UNDERPROMOTION_PENALTY < -MAX_HISTORY_SCORE);

template<Color color>
class MovePicker {
public:
    enum class Stage: int {
        TT_MOVE,
        GEN_NOISIES,
        GOOD_NOISIES,
        GEN_QUIETS,
        QUIETS,
        BAD_NOISIES,
        DONE
    };

    MovePicker(
        const Board& board_,
        const BoardState<color, LegalPolicy>& board_state_,
        Move tt_move_ = MOVE_NONE,
        std::array<Move, 2> killers_ = {MOVE_NONE, MOVE_NONE},
        bool include_quiets_ = true,
        const HistoryTable& history_ = EMPTY_HISTORY
    ) :
        board(board_),
        board_state(board_state_),
        tt_move(tt_move_),
        killers(std::move(killers_)),
        include_quiets(include_quiets_),
        history(history_),
        stage(Stage::TT_MOVE)
    {}

    inline Move next() {
        while (true) {
            switch (stage) {
                case Stage::TT_MOVE:
                    stage = Stage::GEN_NOISIES;
                    if (is_legal_move<color>(board, board_state, tt_move)) {
                        return tt_move;
                    }
                    tt_move = MOVE_NONE;
                case Stage::GEN_NOISIES:
                    current = 0;
                    moves = generate_moves<color, LegalPolicy, NoisyMoves>(board, board_state);
                    for (size_t i = 0; i < moves.size(); ++i) {
                        move_scores[i] = noisy_gain(moves[i]);
                    }
                    num_noisies = good_noisies_end = moves.size();
                    stage = Stage::GOOD_NOISIES;
                case Stage::GOOD_NOISIES:
                    
                    while (current < good_noisies_end) {
                        swap_best_move_to_current(good_noisies_end);
                        Move& move = moves[current];
                        if (move == tt_move) {
                            ++current;
                            continue;
                        }

                        if (move_scores[current] >= 0) {
                            return moves[current++];
                        } else if (!is_promotion(move.move_type)) {
                            move_scores[current] = compute_see(move);
                            if (move_scores[current] >= 0) {
                                return moves[current++];
                            }
                        }

                        --good_noisies_end;
                        std::swap(moves[current], moves[good_noisies_end]);
                        std::swap(move_scores[current], move_scores[good_noisies_end]);
                    }
                    stage = Stage::GEN_QUIETS;
                case Stage::GEN_QUIETS:
                    if (!include_quiets) {
                        stage = Stage::BAD_NOISIES;
                        break;
                    }

                    current = num_noisies;
                    moves.append(generate_moves<color, LegalPolicy, QuietMoves>(board, board_state));

                    for (size_t i = current; i < moves.size(); ++i) {
                        const Move& move = moves[i];
                        if (move == killers[0]) {
                            move_scores[i] = KILLER_0_BONUS;
                        } else if (move == killers[1]) {
                            move_scores[i] = KILLER_1_BONUS;
                        } else {
                            move_scores[i] = history[idx(move.from)][idx(move.to)];
                        }
                    }

                    stage = Stage::QUIETS;
                case Stage::QUIETS:
                    while (current < moves.size()) {
                        swap_best_move_to_current(moves.size());
                        if (moves[current] != tt_move) {
                            return moves[current++];
                        }
                        current += 1;
                    }

                    moves.resize(num_noisies);
                    current = good_noisies_end;
                    stage = Stage::BAD_NOISIES;
                case Stage::BAD_NOISIES:
                    while (current < moves.size()) {
                        swap_best_move_to_current(num_noisies);
                        if (moves[current] != tt_move) {
                            return moves[current++];
                        }
                        current += 1;
                    }
                    stage = Stage::DONE;
                case Stage::DONE:
                    return MOVE_NONE;
            }
        }
    }

    inline Stage current_stage() const {
        return stage;
    }

private:
    class ScoredMoves {
        Move move;
        int16_t score;
    };

    const Board& board;
    const BoardState<color, LegalPolicy>& board_state;
    Move tt_move;
    std::array<Move, 2> killers;
    bool include_quiets;
    const HistoryTable& history;

    Stage stage;
    size_t current;
    size_t good_noisies_end;
    size_t num_noisies;
    MoveList moves;
    std::array<int16_t, MoveList::max_length> move_scores;

    inline int16_t noisy_gain(const Move& move) {
        assert(is_capture(move.move_type) || is_promotion(move.move_type));
        // MVV-LVA
        if (is_promotion(move.move_type)) {
            if (promote_to(move.move_type) != Piece::QUEEN) {
                return UNDERPROMOTION_PENALTY;
            }
            // promotion may not be capture, captured_piece may be none
            const Piece captured_piece = board.get_piece_at<false>(move.to);
            return PIECE_VALUES[idx(captured_piece)] + PIECE_VALUES[idx(promote_to(move.move_type))] - PAWN_VALUE;
        } else {
            // always a capture
            const Piece captured_piece = (move.move_type == MoveType::EP_CAPTURE ? Piece::PAWN : board.get_piece_at(move.to));
            return PIECE_VALUES[idx(captured_piece)] - PIECE_VALUES[idx(board.get_piece_at(move.from))];
        }
    }

    inline int16_t compute_see(const Move& move) {
        // square undefended, shortcut check using board_state
        if (zero(bb_square(move.to) & board_state.king_unallowed)) {
            return PIECE_VALUES[idx(board.get_piece_at(move.to))];;
        }
        return static_exchange_evaluation<color>(board, move);
    }

    inline void swap_best_move_to_current(size_t active_size) {
        size_t best = current;
        for (size_t i = current + 1; i < active_size; ++i) {
            if (move_scores[i] > move_scores[best]) {
                best = i;
            }
        }
        std::swap(moves[current], moves[best]);
        std::swap(move_scores[current], move_scores[best]);
    }
};

} // namespace bears_chess
