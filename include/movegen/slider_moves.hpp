#pragma once
#include "types.hpp"
#include "movelist.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/board_state.hpp"
#include "movegen/policy.hpp"

namespace bears_chess {

template<Color color, Piece move_type, LegalityPolicy Policy, MoveSelection Selection>
void generate_slider_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
)
requires is_bishop_or_rook<move_type>
{
    constexpr Color opponent_color = ~color;

    Bitboard bb_occupied = board.occupied;
    Bitboard bb_quiet = ~bb_occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];
    if constexpr (Policy::enforce_evasions) {
        bb_quiet &= state.evasion_mask;
        bb_capture &= state.evasion_mask;
    }
    
    Bitboard bb_sliders = (
        board.pieces[idx(color)][idx(move_type)] |
        board.pieces[idx(color)][idx(Piece::QUEEN)]
    );

    if constexpr (Policy::enforce_pins) {
        Bitboard bb_pinned_sliders = bb_sliders & state.pinned;
        Square king_square = board.king_sq[idx(color)];
        for (Square from : BBSquareScan(bb_pinned_sliders)) {
            IndexDirection pin_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(from)];
            Bitboard bb_moves = bb_attacks<move_type>(from, bb_occupied) & state.pin_rays[idx(pin_dir)];
            if constexpr (Selection::include_quiets) {
                moves.append_bb(from, bb_moves & bb_quiet, MoveType::QUIET);
            }
            if constexpr (Selection::include_captures) {
                moves.append_bb(from, bb_moves & bb_capture, MoveType::CAPTURE);
            }
        }

        bb_sliders &= ~bb_pinned_sliders;
    }

    for (Square from : BBSquareScan(bb_sliders)) {
        Bitboard bb_moves = bb_attacks<move_type>(from, bb_occupied);
        if constexpr (Selection::include_quiets) {
            moves.append_bb(from, bb_moves & bb_quiet, MoveType::QUIET);
        }
        if constexpr (Selection::include_captures) {
            moves.append_bb(from, bb_moves & bb_capture, MoveType::CAPTURE);
        }
    }
}

template<Color color, LegalityPolicy Policy, MoveSelection Selection>
inline void generate_slider_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
) {
    generate_slider_moves<color, Piece::ROOK, Policy, Selection>(board, moves, state);
    generate_slider_moves<color, Piece::BISHOP, Policy, Selection>(board, moves, state);
}

} // namespace bears_chess
