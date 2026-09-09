#pragma once
#include "bears_chess/types.hpp"
#include "bears_chess/bitboard.hpp"
#include "bears_chess/board.hpp"
#include "bears_chess/movegen/movelist.hpp"
#include "bears_chess/movegen/policy.hpp"
#include "bears_chess/movegen/board_state.hpp"

namespace bears_chess {

template<Color color, LegalityPolicy Policy, MoveSelection Selection>
inline void generate_king_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_quiet = ~board.occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];

    if constexpr (Policy::enforce_king_safety) {
        Bitboard bb_unattacked = ~state.king_unallowed;
        bb_quiet &= bb_unattacked;
        bb_capture &= bb_unattacked;
    }

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = bb_attacks<Piece::KING>(from);
    if constexpr (Selection::include_quiets) {
        moves.append_bb(from, bb_moves & bb_quiet, MoveType::QUIET);
    }
    if constexpr (Selection::include_captures) {
        moves.append_bb(from, bb_moves & bb_capture, MoveType::CAPTURE);
    }
}


template<Color color, LegalityPolicy Policy, MoveSelection Selection>
inline void generate_castle_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
) {
    if constexpr (!Selection::include_quiets) {
        return;
    }

    constexpr Bitboard bb_kingside = BB_CASTLE_PATHS<Piece::KING>[idx(color)];
    constexpr Bitboard bb_queenside = BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)];
    constexpr Move kingside_move = CASTLE_MOVES<Piece::KING>[idx(color)];
    constexpr Move queenside_move = CASTLE_MOVES<Piece::QUEEN>[idx(color)];
    constexpr Bitboard bb_attack_block = ~bb_file(File::B);

    CastlingRights rights = board.castling_rights;
    if (!castling_allowed<color>(rights)) {
        return;
    }

    if constexpr (Policy::enforce_evasions) {
        if (nonzero(state.checkers)) {
            return;
        }
    }

    Bitboard bb_blocked = board.occupied;
    if constexpr (Policy::enforce_king_safety) {
        bb_blocked |= state.king_unallowed & bb_attack_block;
    }

    if (castling_allowed<color, Piece::KING>(rights) && zero(bb_kingside & bb_blocked)) {
        moves.emplace_back(kingside_move);
    }
    if (castling_allowed<color, Piece::QUEEN>(rights) && zero(bb_queenside & bb_blocked)) {
        moves.emplace_back(queenside_move);
    }
}

} // namespace bears_chess
