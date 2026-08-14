#pragma once

#include "types.hpp"
#include "movegen.hpp"
#include "evaluation.hpp"

namespace bears_chess {

template<Color color>
class MovePicker {
public:
    enum class Stage: int {
        TT_MOVE,
        GEN_CAPTURES,
        GOOD_CAPTURES,
        GEN_QUIETS,
        QUIETS,
        BAD_CAPTURES,
        DONE
    };

    MovePicker(
        const Board& board_, const BoardState<color, LegalPolicy>& board_state_,
        Move tt_move_ = MOVE_NONE, std::array<Move, 2> killers_ = {MOVE_NONE, MOVE_NONE},
        bool include_quiets_ = true
    ) :
        board(board_),
        board_state(board_state_),
        tt_move(tt_move_),
        killers(std::move(killers_)),
        include_quiets(include_quiets_),
        stage(Stage::TT_MOVE)
    {}

    inline Move next() {
        while (true) {
            switch (stage) {
                case Stage::TT_MOVE:
                    stage = Stage::GEN_CAPTURES;
                    if (is_legal_move<color>(board, board_state, tt_move)) {
                        return tt_move;
                    }
                    tt_move = MOVE_NONE;
                case Stage::GEN_CAPTURES:
                    current = 0;
                    moves = generate_moves<color, LegalPolicy, CaptureMoves>(board, board_state);
                    for (size_t i = 0; i < moves.size(); ++i) {
                        capture_scores[i] = capture_gain(moves[i]);
                    }
                    num_captures = good_captures_end = moves.size();
                    stage = Stage::GOOD_CAPTURES;
                case Stage::GOOD_CAPTURES:
                    
                    while (current < good_captures_end) {
                        swap_best_capture_to_current(good_captures_end);
                        Move& move = moves[current];
                        if (move == tt_move) {
                            ++current;
                            continue;
                        }
                        if (capture_scores[current] >= 0 ||
                            (capture_scores[current] = compute_see(move)) >= 0)
                        {
                            return moves[current++];
                        }

                        --good_captures_end;
                        std::swap(moves[current], moves[good_captures_end]);
                        std::swap(capture_scores[current], capture_scores[good_captures_end]);
                    }
                    stage = Stage::GEN_QUIETS;
                case Stage::GEN_QUIETS:
                    if (!include_quiets) {
                        stage = Stage::BAD_CAPTURES;
                        break;
                    }

                    current = num_captures;
                    moves.append(generate_moves<color, LegalPolicy, QuietMoves>(board, board_state));

                    // promote killer moves to front
                    {
                        size_t front = current;
                        for (const Move& killer : killers) {
                            if (killer.is_none() || killer == tt_move) {
                                continue;
                            }
                            if (moves.move_to(killer, front)) {
                                ++front;
                            }
                        }
                    }
                    stage = Stage::QUIETS;
                case Stage::QUIETS:
                    while (current < moves.size()) {
                        if (moves[current] != tt_move) {
                            return moves[current++];
                        }
                        current += 1;
                    }
                    moves.resize(num_captures);
                    current = good_captures_end;
                    stage = Stage::BAD_CAPTURES;
                case Stage::BAD_CAPTURES:
                    while (current < moves.size()) {
                        swap_best_capture_to_current(num_captures);
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

    Stage stage;
    size_t current;
    size_t good_captures_end;
    size_t num_captures;
    MoveList moves;
    std::array<int16_t, MoveList::max_length> capture_scores;

    inline int16_t capture_gain(const Move& move) {
        assert(is_capture(move.move_type));
        const Piece captured_piece = (move.move_type == MoveType::EP_CAPTURE ? Piece::PAWN : board.get_piece_at(move.to));
        // MVV-LVA
        return PIECE_VALUES[idx(captured_piece)] - PIECE_VALUES[idx(board.get_piece_at(move.from))];
    }

    inline int16_t compute_see(const Move& move) {
        // square undefended, shortcut check using board_state
        if (zero(bb_square(move.to) & board_state.king_unallowed)) {
            return PIECE_VALUES[idx(board.get_piece_at(move.to))];;
        }
        return static_exchange_evaluation<color>(board, move);
    }

    inline void swap_best_capture_to_current(size_t active_size) {
        size_t best = current;
        for (size_t i = current + 1; i < active_size; ++i) {
            if (capture_scores[i] > capture_scores[best]) {
                best = i;
            }
        }
        std::swap(moves[current], moves[best]);
        std::swap(capture_scores[current], capture_scores[best]);
    }
};

} // namespace bears_chess