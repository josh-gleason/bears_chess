#pragma once

#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"

namespace bears_chess {

struct BoardCache {
    Bitboard checkers;
    Bitboard opponent_attacks;      // enemy attack mask ignoring our king
    Bitboard block_mask;            // non-king moves are restricted to these squares to block checkers if present
    Bitboard pinned_pieces;         // all pieces that are pinned
    Bitboard pin_masks[num_of<IndexDirection>];     // legal move mask for pinned piece, indexed by direction from king
};

inline void append_moves(MoveList& moves, Square from, Bitboard to_squares, MoveType move_type) {
    for (Square to : BBSquareScan(to_squares)) {
        moves.emplace_back(from, to, move_type);
    }
}

template<Color color>
inline void generate_legal_king_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color opponent_color = ~color;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(opponent_color)];

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = bb_attacks<Piece::KING>(from);
    Bitboard bb_quiet = (bb_moves & unoccupied & ~cache.opponent_attacks);
    Bitboard bb_capture = (bb_moves & opponent_occupied & ~cache.opponent_attacks);
    append_moves(moves, from, bb_quiet, MoveType::QUIET);
    append_moves(moves, from, bb_capture, MoveType::CAPTURE);
}

template<Color color>
inline void generate_legal_knight_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color opponent_color = ~color;
    
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)] & ~cache.pinned_pieces;
    Bitboard legal_quiet = ~board.occupied & cache.block_mask;
    Bitboard legal_capture = board.occupied_by_color[idx(opponent_color)] & cache.block_mask;

    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = bb_attacks<Piece::KNIGHT>(from);
        Bitboard bb_quiet = (bb_moves & legal_quiet);
        Bitboard bb_capture = (bb_moves & legal_capture);
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
}

template<Color color, Piece move_type> requires is_bishop_or_rook<move_type>
inline void generate_legal_slider_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color opponent_color = ~color;

    Bitboard occupied = board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(opponent_color)];
    Bitboard bb_sliders = board.pieces[idx(color)][idx(move_type)] | board.pieces[idx(color)][idx(Piece::QUEEN)];

    Bitboard pinned_sliders = bb_sliders & cache.pinned_pieces;
    Bitboard unpinned_sliders = bb_sliders & ~cache.pinned_pieces;

    Bitboard legal_quiet = ~occupied & cache.block_mask;
    Bitboard legal_capture = opponent_occupied & cache.block_mask;

    Square king_square = board.king_sq[idx(color)];

    for (Square from : BBSquareScan(pinned_sliders)) {
        IndexDirection pin_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(from)];
        Bitboard bb_moves = bb_attacks<move_type>(from, occupied) & cache.pin_masks[idx(pin_dir)];
        Bitboard bb_quiet = bb_moves & legal_quiet;
        Bitboard bb_capture = bb_moves & legal_capture;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }

    for (Square from : BBSquareScan(unpinned_sliders)) {
        Bitboard bb_moves = bb_attacks<move_type>(from, occupied);
        Bitboard bb_quiet = bb_moves & legal_quiet;
        Bitboard bb_capture = bb_moves & legal_capture;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
}

template<Color color>
inline void generate_legal_ep_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color opponent_color = ~color;

    Square king_sq = board.king_sq[idx(color)];
    Bitboard bb_king = bb_square(king_sq);

    Square to = board.ep_square;
    Bitboard bb_opponent_pawn = BB_SINGLE_PAWN_MOVES[idx(opponent_color)][idx(to)];
    if (nonzero(cache.block_mask & bb_opponent_pawn)) {
        // pawn is allowed to be captured
        Bitboard bb_ep_pawn_mask = bb_attacks<opponent_color, Piece::PAWN>(to);
        Bitboard bb_capture_ep = bb_ep_pawn_mask & board.pieces[idx(color)][idx(Piece::PAWN)];

        for (Square from : BBSquareScan(bb_capture_ep)) {
            Bitboard legal_mask = bb_square(to);
            Bitboard bb_from = bb_square(from);

            if (nonzero(cache.pinned_pieces & bb_from)) {
                IndexDirection pin_dir = DIR_BETWEEN<IndexDirection>[idx(king_sq)][idx(from)];
                legal_mask &= cache.pin_masks[idx(pin_dir)];
            }
            if (nonzero(legal_mask)) {
                Bitboard exposing_rank = bb_rank(bitscan_forward(bb_opponent_pawn));
                bool exposing = nonzero(bb_king & exposing_rank);
                if (exposing) {
                    Bitboard opponent_sliders = exposing_rank & (
                        board.pieces[idx(opponent_color)][idx(Piece::ROOK)] | board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
                    );
                    if (nonzero(opponent_sliders)) {
                        // check if after removing pawns if king is in check
                        Bitboard attack = bb_attacks<Piece::ROOK>(king_sq, board.occupied & ~(bb_opponent_pawn | bb_from));
                        exposing = nonzero(opponent_sliders & attack);
                    } else {
                        exposing = false;
                    }
                }
                if (!exposing) {
                    moves.emplace_back(from, to, MoveType::EP_CAPTURE);
                }
            }
        }
    }
}


template<Direction move_dir, MoveType type, Color color>
inline void emplace_pawn_moves(MoveList& moves, Bitboard bb_to) {
    constexpr Bitboard bb_promote = (color == Color::WHITE ? bb_rank(Rank::_8) : bb_rank(Rank::_1));
    constexpr Bitboard bb_nopromote = ~bb_promote;
    constexpr Direction dir_reverse = static_cast<Direction>(-idx(move_dir));
    constexpr MoveType type_promote_queen = type | MoveType::QUEEN_PROMOTION;
    constexpr MoveType type_promote_rook = type | MoveType::ROOK_PROMOTION;
    constexpr MoveType type_promote_knight = type | MoveType::KNIGHT_PROMOTION;
    constexpr MoveType type_promote_bishop = type | MoveType::BISHOP_PROMOTION;
    
    for (Square to : BBSquareScan(bb_to & bb_nopromote)) {
        Square from = static_cast<Square>(idx(to) + idx(dir_reverse));
        moves.emplace_back(from, to, type);
    }
    for (Square to : BBSquareScan(bb_to & bb_promote)) {
        Square from = static_cast<Square>(idx(to) + idx(dir_reverse));
        moves.emplace_back(from, to, type_promote_queen);
        moves.emplace_back(from, to, type_promote_rook);
        moves.emplace_back(from, to, type_promote_knight);
        moves.emplace_back(from, to, type_promote_bishop);
    }
}

template<Direction dir> requires is_ordinal<dir>
inline Bitboard get_square_diag(Square king_sq) {
    if constexpr (dir == Direction::NORTHEAST || dir == Direction::SOUTHWEST) {
        return BB_DIAG45_OF[idx(king_sq)];
    } else {
        return BB_DIAG135_OF[idx(king_sq)];
    }
}

template<>
inline void generate_legal_ep_moves<Color::WHITE>(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color color = Color::WHITE;
    constexpr Color opponent_color = ~color;

    Square to = board.ep_square;
    Bitboard bb_to = bb_square(to);
    Bitboard bb_opponent_pawn = bb_shift<Direction::SOUTH>(bb_to);
    if (zero((bb_to | bb_opponent_pawn) & cache.block_mask)) {
        return;
    }

    Square king_sq = board.king_sq[idx(color)];

    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_from_west = bb_pawns & bb_shift<Direction::SOUTHWEST, true>(bb_to);
    Bitboard bb_pawn_from_east = bb_pawns & bb_shift<Direction::SOUTHEAST, true>(bb_to);

    if (rank_of(king_sq) == Rank::_5) {
        Bitboard bb_pawn_mask = bb_opponent_pawn | bb_pawn_from_west | bb_pawn_from_east;
        if (popcount(bb_pawn_mask) == 2) {
            Bitboard bb_opponent_rooks = bb_rank(Rank::_5) & (
                board.pieces[idx(opponent_color)][idx(Piece::ROOK)]
                | board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
            );
            if (nonzero(bb_opponent_rooks)) {
                Bitboard bb_rook_pinners = bb_attacks<Piece::ROOK>(king_sq, board.occupied & ~bb_pawn_mask) & bb_opponent_rooks;
                if (nonzero(bb_rook_pinners)) {
                    return;
                }
            }
        }
    }
    // if (rank_of(king_sq) == Rank::_5 && nonzero(bb_opponent_rooks) && popcount(bb_pawn_mask) == 2 &&
    //     nonzero(bb_attacks<Piece::ROOK>(king_sq, board.occupied & ~bb_pawn_mask) & bb_opponent_rooks)
    // ) {
    //     return;
    // }

    Bitboard bb_pinned = cache.pinned_pieces;
    Bitboard bb_unpinned = ~bb_pinned;

    Bitboard bb_allow_from_west = bb_unpinned | (bb_pinned & get_square_diag<Direction::SOUTHWEST>(king_sq));
    Bitboard bb_allow_from_east = bb_unpinned | (bb_pinned & get_square_diag<Direction::SOUTHEAST>(king_sq));

    Bitboard bb_ep_from_west = bb_allow_from_west & bb_pawn_from_west;
    Bitboard bb_ep_from_east = bb_allow_from_east & bb_pawn_from_east;

    if (nonzero(bb_ep_from_west))
        moves.emplace_back(
            static_cast<Square>(idx(to) + idx(Direction::SOUTHWEST)),
            to,
            MoveType::EP_CAPTURE
        );
    if (nonzero(bb_ep_from_east))
        moves.emplace_back(
            static_cast<Square>(idx(to) + idx(Direction::SOUTHEAST)),
            to,
            MoveType::EP_CAPTURE
        );
}

template<Color color>
inline void generate_legal_pawn_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    constexpr Color opponent_color = ~color;

    constexpr Direction push_dir = (color == Color::WHITE ? Direction::NORTH : Direction::SOUTH);
    constexpr Direction dbl_push_dir = (color == Color::WHITE ? Direction::NORTHNORTH : Direction::SOUTHSOUTH);
    constexpr Direction attack_dir_east = (color == Color::WHITE ? Direction::NORTHEAST : Direction::SOUTHEAST);
    constexpr Direction attack_dir_west = (color == Color::WHITE ? Direction::NORTHWEST : Direction::SOUTHWEST);
    constexpr Bitboard bb_dbl_rank = (color == Color::WHITE ? bb_rank(Rank::_4) : bb_rank(Rank::_5));
        
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pinned = cache.pinned_pieces;
    Bitboard bb_unpinned = ~bb_pinned;

    Square king_sq = board.king_sq[idx(color)];
    Bitboard bb_allow_east = bb_unpinned | (bb_pinned & get_square_diag<attack_dir_east>(king_sq));
    Bitboard bb_allow_west = bb_unpinned | (bb_pinned & get_square_diag<attack_dir_west>(king_sq));

    Bitboard opponent_capturable = cache.block_mask & board.occupied_by_color[idx(opponent_color)];
    Bitboard bb_attack_east = opponent_capturable & bb_shift<attack_dir_east, true>(bb_pawns & bb_allow_east);
    Bitboard bb_attack_west = opponent_capturable & bb_shift<attack_dir_west, true>(bb_pawns & bb_allow_west);
    
    Bitboard unoccupied = ~board.occupied;
    Bitboard bb_allow_push = bb_unpinned | (bb_pinned & BB_FILE_OF[idx(king_sq)]);
    Bitboard bb_single = unoccupied & bb_shift<push_dir>(bb_pawns & bb_allow_push);
    Bitboard bb_push = cache.block_mask & bb_single;
    Bitboard bb_dbl_push = bb_dbl_rank & cache.block_mask & unoccupied & bb_shift<push_dir>(bb_single);

    emplace_pawn_moves<attack_dir_east, MoveType::CAPTURE, color>(moves, bb_attack_east);
    emplace_pawn_moves<attack_dir_west, MoveType::CAPTURE, color>(moves, bb_attack_west);
    emplace_pawn_moves<push_dir, MoveType::QUIET, color>(moves, bb_push);

    for (Square to: BBSquareScan(bb_dbl_push)) {
        Square from = static_cast<Square>(idx(to) - idx(dbl_push_dir));
        moves.emplace_back(from, to, MoveType::DOUBLE_PAWN_PUSH);
    }

    if (board.ep_square != Square::NONE) {
        generate_legal_ep_moves<color>(board, moves, cache);
    }
}

template<Color color>
inline void generate_legal_castle_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    if (nonzero(cache.checkers)) {
        return;
    }

    Bitboard occupied = board.occupied;
    CastlingRights castling_rights = board.castling_rights;

    // b-file attacks dont prevent castle
    Bitboard occupied_or_attacked = occupied | (cache.opponent_attacks & ~bb_file(File::B));

    if (castling_allowed<color, Piece::KING>(castling_rights) && zero(BB_CASTLE_PATHS<Piece::KING>[idx(color)] & occupied_or_attacked)) {
        moves.emplace_back(CASTLE_MOVES<Piece::KING>[idx(color)]);
    }
    if (castling_allowed<color, Piece::QUEEN>(castling_rights) && zero(BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)] & occupied_or_attacked)) {
        moves.emplace_back(CASTLE_MOVES<Piece::QUEEN>[idx(color)]);
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

    Bitboard attacks = Bitboard::EMPTY;
    Bitboard occupied = board.occupied;
    
    if constexpr (omit_king) {
        occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
    }

    attacks |= generate_knight_attacks<opponent_color>(board);
    attacks |= generate_slider_attacks<opponent_color, Piece::ROOK>(board, occupied);
    attacks |= generate_slider_attacks<opponent_color, Piece::BISHOP>(board, occupied);
    attacks |= generate_pawn_attacks<opponent_color>(board);
    attacks |= generate_king_attacks<opponent_color>(board);

    return attacks;
}

template<Color color, bool test_attacks=false>
inline Bitboard calculate_checkers(const Board& board, Bitboard opponent_attacks = Bitboard::EMPTY) {
    constexpr Color opponent_color = ~color;
    Square king_square = board.king_sq[idx(color)];

    if constexpr (test_attacks) {
        // allow early exiting, valid either way
        Bitboard bb_king = bb_square(king_square);
        if (zero(opponent_attacks & bb_king)) {
            return Bitboard::EMPTY;
        }
    }

    Bitboard opponent_pawns = board.pieces[idx(opponent_color)][idx(Piece::PAWN)];
    Bitboard opponent_knights = board.pieces[idx(opponent_color)][idx(Piece::KNIGHT)];
    Bitboard opponent_queens = board.pieces[idx(opponent_color)][idx(Piece::QUEEN)];
    Bitboard opponent_rooks = board.pieces[idx(opponent_color)][idx(Piece::ROOK)];
    Bitboard opponent_bishops = board.pieces[idx(opponent_color)][idx(Piece::BISHOP)];

    Bitboard knight_checkers = bb_attacks<Piece::KNIGHT>(king_square) & opponent_knights;
    Bitboard pawn_checkers = bb_attacks<color, Piece::PAWN>(king_square) & opponent_pawns;
    Bitboard rook_checkers = bb_attacks<Piece::ROOK>(king_square, board.occupied) & (opponent_rooks | opponent_queens);
    Bitboard bishop_checkers = bb_attacks<Piece::BISHOP>(king_square, board.occupied) & (opponent_bishops | opponent_queens);

    return knight_checkers | pawn_checkers | rook_checkers | bishop_checkers;
}

template<Color color, Piece move_type>
requires is_bishop_or_rook<move_type>
Bitboard calculate_pinned_pieces(const Board& board, Bitboard pin_masks[num_of<IndexDirection>], Bitboard& block_check) {
    Bitboard pinned = Bitboard::EMPTY;

    Square king_square = board.king_sq[idx(board.side_to_move)];
    constexpr Color opponent_color = ~color;

    Bitboard enemy_sliders = (
        board.pieces[idx(opponent_color)][idx(move_type)]
        | board.pieces[idx(opponent_color)][idx(Piece::QUEEN)]
    );

    for (auto pinner_sq : BBSquareScan(enemy_sliders)) {
        Bitboard between = BB_RAY<move_type>[idx(pinner_sq)][idx(king_square)];

        Bitboard my_pieces_between = between & board.occupied_by_color[idx(color)];
        Bitboard opponent_pieces_between = between & board.occupied_by_color[idx(opponent_color)];

        int my_piece_count = popcount(my_pieces_between);
        int opponent_piece_count = popcount(opponent_pieces_between);

        if (opponent_piece_count == 1) {
            if (my_piece_count == 0) {
                // king is checked by a slider
                block_check |= between;
            } else if (my_piece_count == 1) {
                IndexDirection pinner_dir = DIR_BETWEEN<IndexDirection>[idx(king_square)][idx(pinner_sq)];
                pin_masks[idx(pinner_dir)] = between;
                pinned |= my_pieces_between;
            }
        }

    }

    return pinned;
}

} // namespace bears_chess