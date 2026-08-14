#pragma once
#include "types.hpp"
#include "board.hpp"
#include "movegen/movelist.hpp"
#include "movegen/policy.hpp"
#include "movegen/board_state.hpp"
#include "movegen/king_moves.hpp"
#include "movegen/knight_moves.hpp"
#include "movegen/slider_moves.hpp"
#include "movegen/pawn_moves.hpp"

namespace bears_chess {

struct PseudoLegalPolicy {
    static constexpr bool enforce_king_safety = false;
    static constexpr bool enforce_evasions = false;
    static constexpr bool enforce_pins = false;
};

struct LegalPolicy {
    static constexpr bool enforce_king_safety = true;
    static constexpr bool enforce_evasions = true;
    static constexpr bool enforce_pins = true;
};

struct CaptureMoves {
    static constexpr bool include_captures = true;
    static constexpr bool include_quiets = false;
};

struct QuietMoves {
    static constexpr bool include_captures = false;
    static constexpr bool include_quiets = true;
};

struct AllMoves {
    static constexpr bool include_captures = true;
    static constexpr bool include_quiets = true;
};

template<Color color, LegalityPolicy Policy, MoveSelection Selection=AllMoves>
inline MoveList generate_moves(const Board& board, const BoardState<color, Policy>& state) {
    MoveList moves;
    generate_king_moves<color, Policy, Selection>(board, moves, state);

    if constexpr (Policy::enforce_evasions) {
        if (state.num_checkers >= 2) {
            return moves;
        }
    }

    generate_knight_moves<color, Policy, Selection>(board, moves, state);
    generate_slider_moves<color, Policy, Selection>(board, moves, state);
    generate_pawn_moves<color, Policy, Selection>(board, moves, state);
    generate_castle_moves<color, Policy, Selection>(board, moves, state);

    return moves;
}


template<Color color, LegalityPolicy Policy, MoveSelection Selection=AllMoves>
inline MoveList generate_moves(const Board& board) {
    BoardState<color, Policy> state(board);
    return generate_moves<color, Policy, Selection>(board, state);
}


template <LegalityPolicy Policy, MoveSelection Selection=AllMoves>
MoveList generate_moves(const Board& board) {
    if (board.side_to_move == Color::WHITE)
        return generate_moves<Color::WHITE, Policy, Selection>(board);
    return generate_moves<Color::BLACK, Policy, Selection>(board);
}


template<Color color>
bool is_legal_move(const Board& board, const BoardState<color, LegalPolicy>& state, const Move& move) {
    // assumes that moves are either none or come from a valid movegen, but may be a different color due
    // to TT collision

    constexpr Bitboard bb_kingside = BB_CASTLE_PATHS<Piece::KING>[idx(color)];
    constexpr Bitboard bb_queenside = BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)];
    constexpr Bitboard bb_attack_block = ~bb_file(File::B);
    constexpr Rank promo_rank = (color == Color::WHITE ? Rank::_8 : Rank::_1);
    constexpr Rank castle_rank = (color == Color::WHITE ? Rank::_1 : Rank::_8);
    constexpr Direction dir_push = (color == Color::WHITE ? Direction::NORTH : Direction::SOUTH);

    if (move.is_none()) {
        return false;
    }

    if (move.move_type == MoveType::KING_CASTLE) {
        CastlingRights rights = board.castling_rights;
        if (!castling_allowed<color, Piece::KING>(rights) || nonzero(state.checkers) || rank_of(move.from) != castle_rank) {
            return false;
        }
        Bitboard bb_blocked = board.occupied | (state.king_unallowed & bb_attack_block);
        return zero(bb_kingside & bb_blocked);
    }
    if (move.move_type == MoveType::QUEEN_CASTLE) {
        CastlingRights rights = board.castling_rights;
        if (!castling_allowed<color, Piece::QUEEN>(rights) || nonzero(state.checkers) || rank_of(move.from) != castle_rank) {
            return false;
        }
        Bitboard bb_blocked = board.occupied | (state.king_unallowed & bb_attack_block);
        return zero(bb_queenside & bb_blocked);
    }

    const Piece piece = board.get_piece_of_color_at<false>(color, move.from);
    if (piece == Piece::NONE) {
        return false;
    }

    if (is_promotion(move.move_type) && (piece != Piece::PAWN || rank_of(move.to) != promo_rank)) {
        return false;
    }

    if (move.move_type == MoveType::DOUBLE_PAWN_PUSH && piece != Piece::PAWN) {
        return false;
    }

    if (move.move_type == MoveType::EP_CAPTURE) {
        MoveList ep_moves;
        generate_ep_moves<color, LegalPolicy, CaptureMoves>(board, ep_moves, state);
        return std::find(ep_moves.begin(), ep_moves.end(), move) != ep_moves.end();
    }

    const Piece piece_to = board.get_piece_at<false>(move.to);
    if (is_capture(move.move_type)) {
        if (piece_to == Piece::NONE || piece_to == Piece::KING || board.get_color_at(move.to) == color) {
            return false;
        }
    } else if (piece_to != Piece::NONE) {
        return false;
    }

    const Bitboard bb_to = bb_square(move.to);
    if (piece == Piece::PAWN) {
        const Bitboard bb_from = bb_square(move.from);
        
        // promotion flag must match arrival on the promotion rank
        if (is_promotion(move.move_type) != (rank_of(move.to) == promo_rank)) {
            return false;
        }

        if (is_capture(move.move_type)) {
            // ep handled earlier
            if (zero(bb_to & bb_attacks<color, Piece::PAWN>(move.from))) {
                return false;
            }
        } else if (move.move_type == MoveType::DOUBLE_PAWN_PUSH) {
            const Bitboard bb_unoccupied = ~board.occupied;
            const Bitboard bb_single = bb_unoccupied & bb_shift<dir_push>(bb_from);
            if (zero(bb_to & bb_shift<dir_push>(bb_single))) {
                return false;
            }
        } else {
            if (zero(bb_to & bb_shift<dir_push>(bb_from))) {
                return false;
            }
        }
    } else {
        Bitboard bb_pattern;
        switch (piece) {
            case Piece::KNIGHT:
                bb_pattern = bb_attacks<Piece::KNIGHT>(move.from);
                break;
            case Piece::BISHOP:
                bb_pattern = bb_attacks<Piece::BISHOP>(move.from, board.occupied);
                break;
            case Piece::ROOK:
                bb_pattern = bb_attacks<Piece::ROOK>(move.from, board.occupied);
                break;
            case Piece::QUEEN:
                bb_pattern = bb_attacks<Piece::QUEEN>(move.from, board.occupied);
                break;
            case Piece::KING:
                bb_pattern = bb_attacks<Piece::KING>(move.from);
                break;
            default:
                return false;
        }
        if (zero(bb_to & bb_pattern)) {
            return false;
        }
    }

    // check & pin
    if (piece == Piece::KING) {
        return zero(bb_to & state.king_unallowed);
    }

    // evasion mask
    if (zero(bb_to & state.evasion_mask)) {
        return false;
    }

    if (nonzero(state.pinned & bb_square(move.from))) {
        const Square king_sq = board.king_sq[idx(color)];
        if (DIR_BETWEEN<IndexDirection>[idx(king_sq)][idx(move.to)] != DIR_BETWEEN<IndexDirection>[idx(king_sq)][idx(move.from)]) {
            return false;
        }
    }

    return true;
}

inline Bitboard attackers_to(const Board& board, Square to_square, Bitboard bb_occupied) {
    // bb_occupied is allowed to be a subset of board.occupied to simulate removed pieces
    const Bitboard bb_queens =
        board.pieces[idx(Color::WHITE)][idx(Piece::QUEEN)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::QUEEN)];
    const Bitboard bb_rooklike_attackers = bb_attacks<Piece::ROOK>(to_square, bb_occupied) & (
        bb_queens |
        board.pieces[idx(Color::WHITE)][idx(Piece::ROOK)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::ROOK)]);
    const Bitboard bb_bishoplike_attackers = bb_attacks<Piece::BISHOP>(to_square, bb_occupied) & (
        bb_queens |
        board.pieces[idx(Color::WHITE)][idx(Piece::BISHOP)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::BISHOP)]);
    const Bitboard bb_knight_attackers = bb_attacks<Piece::KNIGHT>(to_square) & (
        board.pieces[idx(Color::WHITE)][idx(Piece::KNIGHT)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::KNIGHT)]);
    const Bitboard bb_king_attackers = bb_attacks<Piece::KING>(to_square) & (
        board.pieces[idx(Color::WHITE)][idx(Piece::KING)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::KING)]);
    // pawn attacks from to_square locate candidate attacking pawns of the opposite color
    const Bitboard bb_white_pawn_attackers =
        bb_attacks<Color::BLACK, Piece::PAWN>(to_square) &
        board.pieces[idx(Color::WHITE)][idx(Piece::PAWN)];
    const Bitboard bb_black_pawn_attackers =
        bb_attacks<Color::WHITE, Piece::PAWN>(to_square) &
        board.pieces[idx(Color::BLACK)][idx(Piece::PAWN)];
    
    return bb_occupied & (
        bb_rooklike_attackers |
        bb_bishoplike_attackers |
        bb_knight_attackers |
        bb_king_attackers |
        bb_white_pawn_attackers |
        bb_black_pawn_attackers
    );
}

} // namespace bears_chess
