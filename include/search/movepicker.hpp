#pragma once

#include "types.hpp"
#include "movegen.hpp"
#include "score.hpp"

namespace bears_chess {

template<Color color>
class MovePicker {
public:
    enum class Stage: int {
        TT_MOVE,
        GEN_CAPTURES,
        CAPTURES,
        GEN_QUIETS,
        QUIETS,
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
        stage(Stage::TT_MOVE),
        current(0)
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
                        scores[i] = capture_gain(moves[i]);
                    }
                    stage = Stage::CAPTURES;
                case Stage::CAPTURES:
                    while (current < moves.size()) {
                        swap_best_to_current();
                        if (moves[current] != tt_move) {
                            return moves[current++];
                        }
                        current += 1;
                    }
                    stage = Stage::GEN_QUIETS;
                case Stage::GEN_QUIETS:
                    if (!include_quiets) {
                        stage = Stage::DONE;
                        break;
                    }

                    current = 0;
                    moves = generate_moves<color, LegalPolicy, QuietMoves>(board, board_state);

                    // promote killer moves to front
                    {
                        size_t front = 0;
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
    MoveList moves;
    std::array<int16_t, MoveList::max_length> scores;

    inline int16_t capture_gain(const Move& move) {
        const Piece victim = (
            move.move_type == MoveType::EP_CAPTURE
            ? Piece::PAWN
            : board.get_piece_at(move.to)    // assume all moves provided here are captures
        );
        return PIECE_VALUES[idx(victim)] - PIECE_VALUES[idx(board.get_piece_at(move.from))];
    }

    inline void swap_best_to_current() {
        size_t best = current;
        for (size_t i = current + 1; i < moves.size(); ++i) {
            if (scores[i] > scores[best]) {
                best = i;
            }
        }
        std::swap(moves[current], moves[best]);
        std::swap(scores[current], scores[best]);
    }
};

} // namespace bears_chess