#pragma once

#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"
#include "movelist.hpp"

namespace bears_chess {

template<typename T>
concept MoveGenPolicy = requires(const T& policy) {
    { T::enforce_king_safety } -> std::convertible_to<bool>;
    { T::enforce_evasions } -> std::convertible_to<bool>;
    { T::enforce_pins } -> std::convertible_to<bool>;

    requires (!T::enforce_king_safety) || requires {
        { T::king_unallowed } -> std::same_as<Bitboard&>;
        { T::checkers } -> std::same_as<Bitboard&>;
    };
    requires (!T::enforce_evasions) || requires {
        { T::evasion_mask } -> std::same_as<Bitboard&>;
    };
    requires (!T::enforce_pins) || requires {
        { T::pinned } -> std::same_as<Bitboard&>;
        { T::pin_rays } -> std::same_as<Bitboard(&)[num_of<IndexDirection>]>;
    };
};

struct PseudoLegalPolicy {
    static constexpr bool enforce_king_safety = false;
    static constexpr bool enforce_evasions = false;
    static constexpr bool enforce_pins = false;
};

struct LegalPolicy {
    static constexpr bool enforce_king_safety = true;
    static constexpr bool enforce_evasions = true;
    static constexpr bool enforce_pins = true;

    Bitboard king_unallowed;                    // enemy attacks as if our king were not present
    Bitboard checkers;                          // mask of all pieces attacking the king
    Bitboard evasion_mask;                      // non-king moves are restricted to these squares to block checkers if present
    Bitboard pinned;                            // all pieces that are pinned
    Bitboard pin_rays[num_of<IndexDirection>];  // legal move mask for pinned piece, indexed by direction from king
};

inline void append_moves(MoveList& moves, Square from, Bitboard to_squares, MoveType move_type) {
    for (Square to : BBSquareScan(to_squares)) {
        moves.emplace_back(from, to, move_type);
    }
}

template<Color color, MoveGenPolicy Policy>
inline void generate_king_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_quiet = ~board.occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];

    if constexpr (Policy::enforce_king_safety) {
        Bitboard bb_unattacked = ~policy.king_unallowed;
        bb_quiet &= bb_unattacked;
        bb_capture &= bb_unattacked;
    }

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = bb_attacks<Piece::KING>(from);
    append_moves(moves, from, bb_moves & bb_quiet, MoveType::QUIET);
    append_moves(moves, from, bb_moves & bb_capture, MoveType::CAPTURE);
}

template<Color color, MoveGenPolicy Policy>
inline void generate_knight_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_quiet = ~board.occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];
    if constexpr (Policy::enforce_evasions) {
        bb_quiet &= policy.evasion_mask;
        bb_capture &= policy.evasion_mask;
    }

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    if constexpr (Policy::enforce_pins) {
        bb_knights &= ~policy.pinned;
    }

    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = bb_attacks<Piece::KNIGHT>(from);
        append_moves(moves, from, bb_moves & bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_moves & bb_capture, MoveType::CAPTURE);
    }
}

template<Color color, Piece move_type, MoveGenPolicy Policy> requires is_bishop_or_rook<move_type>
void generate_slider_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_occupied = board.occupied;
    Bitboard bb_quiet = ~bb_occupied;
    Bitboard bb_capture = board.occupied_by_color[idx(opponent_color)];
    if constexpr (Policy::enforce_evasions) {
        bb_quiet &= policy.evasion_mask;
        bb_capture &= policy.evasion_mask;
    }
    
    Bitboard bb_sliders = (
        board.pieces[idx(color)][idx(move_type)] |
        board.pieces[idx(color)][idx(Piece::QUEEN)]
    );

    if constexpr (Policy::enforce_pins) {
        Bitboard bb_pinned_sliders = bb_sliders & policy.pinned;
        Square king_square = board.king_sq[idx(color)];
        for (Square from : BBSquareScan(bb_pinned_sliders)) {
            IndexDirection pin_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(from)];
            Bitboard bb_moves = bb_attacks<move_type>(from, bb_occupied) & policy.pin_rays[idx(pin_dir)];
            append_moves(moves, from, bb_moves & bb_quiet, MoveType::QUIET);
            append_moves(moves, from, bb_moves & bb_capture, MoveType::CAPTURE);
        }

        bb_sliders &= ~bb_pinned_sliders;
    }

    for (Square from : BBSquareScan(bb_sliders)) {
        Bitboard bb_moves = bb_attacks<move_type>(from, bb_occupied);
        append_moves(moves, from, bb_moves & bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_moves & bb_capture, MoveType::CAPTURE);
    }
}

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
inline void generate_ep_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;
    constexpr Direction dir_from = (color == Color::WHITE ? Direction::SOUTH : Direction::NORTH);
    constexpr Direction dir_from_west = (color == Color::WHITE ? Direction::SOUTHWEST : Direction::NORTHWEST);
    constexpr Direction dir_from_east = (color == Color::WHITE ? Direction::SOUTHEAST : Direction::NORTHEAST);
    constexpr Rank pawns_rank = (color == Color::WHITE ? Rank::_5 : Rank::_4);
    constexpr Bitboard bb_pawns_rank = bb_rank(pawns_rank);

    Square to = board.ep_square;
    if (to == Square::NONE)
        return;

    Bitboard bb_to = bb_square(to);
    if constexpr (Policy::enforce_evasions) {
        Bitboard bb_opponent_pawn = bb_shift<dir_from>(bb_to);
        if (zero((bb_to | bb_opponent_pawn) & policy.evasion_mask))
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
                    board.pieces[idx(opponent_color)][idx(Piece::ROOK)] | board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
                );
                if (nonzero(bb_opponent_rooks)) {
                    Bitboard bb_rook_pinners = bb_attacks<Piece::ROOK>(king_sq, board.occupied & ~bb_pawn_mask) & bb_opponent_rooks;
                    if (nonzero(bb_rook_pinners))
                        return;
                }
            }
        }

        Bitboard bb_pinned = policy.pinned;
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
inline void generate_pawn_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Color opponent_color = ~color;

    constexpr Direction dir_push = (color == Color::WHITE ? Direction::NORTH : Direction::SOUTH);
    constexpr Direction dir_attack_east = (color == Color::WHITE ? Direction::NORTHEAST : Direction::SOUTHEAST);
    constexpr Direction dir_attack_west = (color == Color::WHITE ? Direction::NORTHWEST : Direction::SOUTHWEST);
    constexpr Bitboard bb_dbl_rank = (color == Color::WHITE ? bb_rank(Rank::_4) : bb_rank(Rank::_5));

    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];

    Bitboard bb_unoccupied = ~board.occupied;
    Bitboard bb_opponent_capturable = board.occupied_by_color[idx(opponent_color)];
    Bitboard bb_unblocked = bb_unoccupied;
    if constexpr (Policy::enforce_evasions) {
        bb_opponent_capturable &= policy.evasion_mask;
        bb_unblocked &= policy.evasion_mask;
    }

    Bitboard bb_attack_east = bb_opponent_capturable;
    Bitboard bb_attack_west = bb_opponent_capturable;
    Bitboard bb_push = bb_unblocked;
    Bitboard bb_dbl_push = bb_unblocked & bb_dbl_rank;

    if constexpr (Policy::enforce_pins) {
        Square king_sq = board.king_sq[idx(color)];
        Bitboard bb_pinned = policy.pinned;
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
    generate_ep_moves<color>(board, moves, policy);
}

template<Color color, MoveGenPolicy Policy>
inline void generate_castle_moves(const Board& board, MoveList& moves, const Policy& policy) {
    constexpr Bitboard bb_kingside = BB_CASTLE_PATHS<Piece::KING>[idx(color)];
    constexpr Bitboard bb_queenside = BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)];
    constexpr Move kingside_move = CASTLE_MOVES<Piece::KING>[idx(color)];
    constexpr Move queenside_move = CASTLE_MOVES<Piece::QUEEN>[idx(color)];
    constexpr Bitboard bb_attack_block = ~bb_file(File::B);

    CastlingRights rights = board.castling_rights;
    if (!castling_allowed<color>(rights)) {
        return;
    }

    if constexpr (Policy::enforce_king_safety) {
        if (nonzero(policy.checkers)) {
            return;
        }
    }

    Bitboard bb_blocked = board.occupied;
    if constexpr (Policy::enforce_king_safety) {
        bb_blocked |= policy.king_unallowed & bb_attack_block;
    }

    if (castling_allowed<color, Piece::KING>(rights) && zero(bb_kingside & bb_blocked)) {
        moves.emplace_back(kingside_move);
    }
    if (castling_allowed<color, Piece::QUEEN>(rights) && zero(bb_queenside & bb_blocked)) {
        moves.emplace_back(queenside_move);
    }
}

template<Color color>
inline Bitboard generate_knight_attacks(const Board& board) {
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    Bitboard bb_knight_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_knights)) {
        bb_knight_attacks |= bb_attacks<Piece::KNIGHT>(from);
    }
    return bb_knight_attacks;
}

template<Color color, Piece move_type> requires is_bishop_or_rook<move_type>
inline Bitboard generate_slider_attacks(const Board& board, Bitboard bb_occupied) {
    Bitboard bb_pieces = (board.pieces[idx(color)][idx(move_type)] | board.pieces[idx(color)][idx(Piece::QUEEN)]);
    Bitboard bb_slider_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pieces)) {
        bb_slider_attacks |= bb_attacks<move_type>(from, bb_occupied);
    }
    return bb_slider_attacks;
}

template<Color color>
inline Bitboard generate_pawn_attacks(const Board& board) {
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pawns)) {
        bb_pawn_attacks |= bb_attacks<color, Piece::PAWN>(from);
    }
    return bb_pawn_attacks;
}

template<Color color>
inline Bitboard generate_king_attacks(const Board& board) {
    return bb_attacks<Piece::KING>(board.king_sq[idx(color)]);
}

template <Color color, bool omit_king=false>
inline Bitboard calculate_opponent_attacks(const Board& board) {
    constexpr Color opponent_color = ~color;

    Bitboard bb_attacks = Bitboard::EMPTY;
    Bitboard bb_occupied = board.occupied;
    
    if constexpr (omit_king) {
        bb_occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
    }

    bb_attacks |= generate_knight_attacks<opponent_color>(board);
    bb_attacks |= generate_slider_attacks<opponent_color, Piece::ROOK>(board, bb_occupied);
    bb_attacks |= generate_slider_attacks<opponent_color, Piece::BISHOP>(board, bb_occupied);
    bb_attacks |= generate_pawn_attacks<opponent_color>(board);
    bb_attacks |= generate_king_attacks<opponent_color>(board);

    return bb_attacks;
}

template<Color color>
inline Bitboard calculate_checkers(const Board& board, Bitboard& bb_block_mask) {
    constexpr Color opponent_color = ~color;
    Square king_sq = board.king_sq[idx(color)];

    Bitboard bb_opponent_pawns = board.pieces[idx(opponent_color)][idx(Piece::PAWN)];
    Bitboard bb_opponent_knights = board.pieces[idx(opponent_color)][idx(Piece::KNIGHT)];
    Bitboard bb_opponent_queens = board.pieces[idx(opponent_color)][idx(Piece::QUEEN)];
    Bitboard bb_opponent_rooklike = (
        board.pieces[idx(opponent_color)][idx(Piece::ROOK)] | bb_opponent_queens
    );
    Bitboard bb_opponent_bishoplike = (
        board.pieces[idx(opponent_color)][idx(Piece::BISHOP)] | bb_opponent_queens
    );

    Bitboard bb_knight_checkers = bb_attacks<Piece::KNIGHT>(king_sq) & bb_opponent_knights;
    Bitboard bb_pawn_checkers = bb_attacks<color, Piece::PAWN>(king_sq) & bb_opponent_pawns;
    Bitboard bb_rook_checkers = bb_attacks<Piece::ROOK>(king_sq, board.occupied) & bb_opponent_rooklike;
    Bitboard bb_bishop_checkers = bb_attacks<Piece::BISHOP>(king_sq, board.occupied) & bb_opponent_bishoplike;

    Bitboard bb_checkers = bb_knight_checkers | bb_pawn_checkers | bb_rook_checkers | bb_bishop_checkers;

    bb_block_mask |= bb_checkers;
    for (Square checker_sq : BBSquareScan(bb_rook_checkers | bb_bishop_checkers)) {
        bb_block_mask |= BB_RAY<move_type>[idx(checker_sq)][idx(king_sq)];
    }

    return bb_checkers;
}

template<Color color>
inline Bitboard calculate_checkers(const Board& board, Bitboard bb_king_unallowed) {
    // same result as calculate_checkers(board) but can exit early if king not in check
    Square king_sq = board.king_sq[idx(color)];
    if (zero(bb_king_unallowed & bb_square(king_sq)))
        return Bitboard::EMPTY;
    return calculate_checkers<color>(board);
}

template<Color color, Piece move_type> requires is_bishop_or_rook<move_type>
inline Bitboard calculate_pinned_pieces(const Board& board, Bitboard bb_pin_rays[num_of<IndexDirection>])
{
    constexpr Color opponent_color = ~color;

    Bitboard bb_pinned = Bitboard::EMPTY;
    Square king_square = board.king_sq[idx(board.side_to_move)];
    Bitboard bb_enemy_sliders = (
        board.pieces[idx(opponent_color)][idx(move_type)] |
        board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
    );

    for (Square pinner_sq : BBSquareScan(bb_enemy_sliders)) {
        Bitboard bb_between = BB_RAY<move_type>[idx(pinner_sq)][idx(king_square)];

        Bitboard bb_opponent_pieces_between = bb_between & board.occupied_by_color[idx(opponent_color)];
        int opponent_piece_count = popcount(bb_opponent_pieces_between);
        if (opponent_piece_count == 1) {
            Bitboard bb_my_pieces_between = bb_between & board.occupied_by_color[idx(color)];
            int my_piece_count = popcount(bb_my_pieces_between);
            if (my_piece_count == 1) {
                IndexDirection pinner_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(pinner_sq)];
                bb_pin_rays[idx(pinner_dir)] = bb_between;
                bb_pinned |= bb_my_pieces_between;
            }
        }
    }

    return bb_pinned;
}

template<Color color>
inline Bitboard calculate_block_mask(const Board& board, Bitboard bb_checkers) {
    if zero(bb_checkers) {
        return Bitboard::FULL;
    }

    Square king_sq = board.king_sq[idx(board.side_to_move)];
    Bitboard bb_occupied = board.occupied;

    Bitboard bb_rook_attack = bb_attacks<Piece::ROOK>(king_sq, bb_occupied);
    Bitboard bb_bishop_attack = bb_attacks<Piece::BISHOP>(king_sq, bb_occupied);

}

} // namespace bears_chess