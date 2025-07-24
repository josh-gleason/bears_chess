#pragma once
#include "types.hpp"
#include "movelist.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movegen/policy.hpp"

namespace bears_chess {


template<Direction move_dir, MoveType type, Color color>
inline void emplace_pawn_moves(MoveList& moves, Bitboard bb_to) {
    constexpr Bitboard bb_promote = (color == Color::WHITE ? bb_rank(Rank::_8) : bb_rank(Rank::_1));
    constexpr Bitboard bb_nopromote = ~bb_promote;
    constexpr MoveType type_promote_queen = type | MoveType::QUEEN_PROMOTION;
    constexpr MoveType type_promote_rook = type | MoveType::ROOK_PROMOTION;
    constexpr MoveType type_promote_knight = type | MoveType::KNIGHT_PROMOTION;
    constexpr MoveType type_promote_bishop = type | MoveType::BISHOP_PROMOTION;
    constexpr int step_size = (type == MoveType::DOUBLE_PAWN_PUSH ? -2 : -1);
    
    if constexpr (type == MoveType::DOUBLE_PAWN_PUSH) {
        for (Square to: BBSquareScan(bb_to)) {
            moves.emplace_back(sq_shift<move_dir, step_size>(to), to, type);
        }
    } else {
        for (Square to : BBSquareScan(bb_to & bb_nopromote)) {
            moves.emplace_back(sq_shift<move_dir, step_size>(to), to, type);
        }
        for (Square to : BBSquareScan(bb_to & bb_promote)) {
            Square from = sq_shift<move_dir, step_size>(to);
            moves.emplace_back(from, to, type_promote_queen);
            moves.emplace_back(from, to, type_promote_rook);
            moves.emplace_back(from, to, type_promote_knight);
            moves.emplace_back(from, to, type_promote_bishop);
        }
    }
}

template<Color color, MoveGenPolicy Policy>
inline void generate_ep_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
) {
    constexpr Color opponent_color = ~color;
    constexpr Direction dir_from = (color == Color::WHITE ? Direction::SOUTH : Direction::NORTH);
    constexpr Direction dir_from_west = (
        color == Color::WHITE ? Direction::SOUTHWEST : Direction::NORTHWEST
    );
    constexpr Direction dir_from_east = (
        color == Color::WHITE ? Direction::SOUTHEAST : Direction::NORTHEAST
    );
    constexpr Rank pawns_rank = (color == Color::WHITE ? Rank::_5 : Rank::_4);
    constexpr Bitboard bb_pawns_rank = bb_rank(pawns_rank);

    Square to = board.ep_square;
    if (to == Square::NONE)
        return;

    Bitboard bb_to = bb_square(to);
    if constexpr (Policy::enforce_evasions) {
        Bitboard bb_opponent_pawn = bb_shift<dir_from>(bb_to);
        if (zero((bb_to | bb_opponent_pawn) & state.evasion_mask))
            return;
    }

    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_from_west = bb_pawns & bb_shift<dir_from_west, true>(bb_to);
    Bitboard bb_pawn_from_east = bb_pawns & bb_shift<dir_from_east, true>(bb_to);

    if constexpr (Policy::enforce_pins) {
        // check if discovered check after EP
        Square king_sq = board.king_sq[idx(color)];
        if (rank_of(king_sq) == pawns_rank) {
            Bitboard bb_opponent_pawn = bb_shift<dir_from>(bb_to);
            Bitboard bb_pawn_mask = bb_opponent_pawn | bb_pawn_from_west | bb_pawn_from_east;
            if (popcount(bb_pawn_mask) == 2) {
                Bitboard bb_opponent_rooks = bb_pawns_rank & (
                    board.pieces[idx(opponent_color)][idx(Piece::ROOK)] |
                    board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
                );
                if (nonzero(bb_opponent_rooks)) {
                    Bitboard bb_rook_pinners = bb_opponent_rooks & bb_attacks<Piece::ROOK>(
                        king_sq,
                        board.occupied & ~bb_pawn_mask
                    );
                    if (nonzero(bb_rook_pinners))
                        return;
                }
            }
        }

        Bitboard bb_pinned = state.pinned;
        Bitboard bb_unpinned = ~bb_pinned;

        Bitboard bb_allow_from_west = bb_unpinned | (bb_pinned & get_diag_of<dir_from_west>(king_sq));
        Bitboard bb_allow_from_east = bb_unpinned | (bb_pinned & get_diag_of<dir_from_east>(king_sq));

        bb_pawn_from_west &= bb_allow_from_west;
        bb_pawn_from_east &= bb_allow_from_east;
    }

    if (nonzero(bb_pawn_from_west))
        moves.emplace_back(sq_shift<dir_from_west>(to), to, MoveType::EP_CAPTURE);
    if (nonzero(bb_pawn_from_east))
        moves.emplace_back(sq_shift<dir_from_east>(to), to, MoveType::EP_CAPTURE);
}

template<Color color, MoveGenPolicy Policy>
inline void generate_pawn_moves(
    const Board& board, MoveList& moves, const BoardState<color, Policy>& state
) {
    constexpr Color opponent_color = ~color;

    constexpr Direction dir_push = (color == Color::WHITE ? Direction::NORTH : Direction::SOUTH);
    constexpr Direction dir_attack_east = (
        color == Color::WHITE ? Direction::NORTHEAST : Direction::SOUTHEAST
    );
    constexpr Direction dir_attack_west = (
        color == Color::WHITE ? Direction::NORTHWEST : Direction::SOUTHWEST
    );
    constexpr Bitboard bb_dbl_rank = (color == Color::WHITE ? bb_rank(Rank::_4) : bb_rank(Rank::_5));

    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];

    Bitboard bb_unoccupied = ~board.occupied;
    Bitboard bb_opponent_capturable = board.occupied_by_color[idx(opponent_color)];
    Bitboard bb_unblocked = bb_unoccupied;
    if constexpr (Policy::enforce_evasions) {
        bb_opponent_capturable &= state.evasion_mask;
        bb_unblocked &= state.evasion_mask;
    }

    Bitboard bb_attack_east = bb_opponent_capturable;
    Bitboard bb_attack_west = bb_opponent_capturable;
    Bitboard bb_push = bb_unblocked;
    Bitboard bb_dbl_push = bb_unblocked & bb_dbl_rank;

    if constexpr (Policy::enforce_pins) {
        Square king_sq = board.king_sq[idx(color)];
        Bitboard bb_pinned = state.pinned;
        Bitboard bb_unpinned = ~bb_pinned;
        Bitboard bb_allow_east = bb_unpinned | (bb_pinned & get_diag_of<dir_attack_east>(king_sq));
        Bitboard bb_allow_west = bb_unpinned | (bb_pinned & get_diag_of<dir_attack_west>(king_sq));
        
        Bitboard bb_allow_push = bb_unpinned | (bb_pinned & BB_FILE_OF[idx(king_sq)]);
        Bitboard bb_single = bb_unoccupied & bb_shift<dir_push>(bb_pawns & bb_allow_push);

        bb_attack_east &= bb_shift<dir_attack_east, true>(bb_pawns & bb_allow_east);
        bb_attack_west &= bb_shift<dir_attack_west, true>(bb_pawns & bb_allow_west);
        bb_push &= bb_single;
        bb_dbl_push &= bb_shift<dir_push>(bb_single);
    } else {
        Bitboard bb_single = bb_unoccupied & bb_shift<dir_push>(bb_pawns);

        bb_attack_east &= bb_shift<dir_attack_east, true>(bb_pawns);
        bb_attack_west &= bb_shift<dir_attack_west, true>(bb_pawns);
        bb_push &= bb_single;
        bb_dbl_push &= bb_shift<dir_push>(bb_single);
    }

    emplace_pawn_moves<dir_attack_east, MoveType::CAPTURE, color>(moves, bb_attack_east);
    emplace_pawn_moves<dir_attack_west, MoveType::CAPTURE, color>(moves, bb_attack_west);
    emplace_pawn_moves<dir_push, MoveType::QUIET, color>(moves, bb_push);
    emplace_pawn_moves<dir_push, MoveType::DOUBLE_PAWN_PUSH, color>(moves, bb_dbl_push);
    generate_ep_moves<color>(board, moves, state);
}

} // namespace bears_chess
