#pragma once

#include "types.hpp"
#include "bitboard.hpp"
#include "board.hpp"

namespace bears_chess {

struct BoardCache {
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

inline void generate_legal_king_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(opponent_color)];

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = bb_attacks<Piece::KING>(from);
    Bitboard bb_quiet = (bb_moves & unoccupied & ~cache.opponent_attacks);
    Bitboard bb_capture = (bb_moves & opponent_occupied & ~cache.opponent_attacks);
    append_moves(moves, from, bb_quiet, MoveType::QUIET);
    append_moves(moves, from, bb_capture, MoveType::CAPTURE);
}

inline void generate_legal_knight_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)] & ~cache.pinned_pieces;
    Bitboard legal_quiet = ~board.occupied & cache.block_mask;
    Bitboard legal_capture = board.occupied_by_color[idx(~color)] & cache.block_mask;

    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = bb_attacks<Piece::KNIGHT>(from);
        Bitboard bb_quiet = (bb_moves & legal_quiet);
        Bitboard bb_capture = (bb_moves & legal_capture);
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
}

template<Piece move_type> requires is_bishop_or_rook<move_type>
inline void generate_legal_slider_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;
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

inline void generate_legal_ep_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Square king_sq = board.king_sq[idx(color)];
    Bitboard bb_king = bb_square(king_sq);

    Square to = board.ep_square;
    Bitboard bb_opponent_pawn = BB_SINGLE_PAWN_MOVES[idx(~color)][idx(to)];
    if (nonzero(cache.block_mask & bb_opponent_pawn)) {
        // pawn is allowed to be captured
        Bitboard bb_ep_pawn_mask = bb_attacks<Piece::PAWN>(~color, to);
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

inline void generate_legal_pawn_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Bitboard occupied = board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(opponent_color)];
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard unoccupied = ~occupied;

    Bitboard legal_quiet = unoccupied & cache.block_mask;
    Bitboard legal_capture = opponent_occupied & cache.block_mask;

    Square king_sq = board.king_sq[idx(color)];
    Bitboard bb_king = bb_square(king_sq);

    for (Square from : BBSquareScan(bb_pawns)) {
        Bitboard bb_from = bb_square(from);
        Bitboard legal_mask = Bitboard::FULL;
        if (nonzero(cache.pinned_pieces & bb_from)) {
            IndexDirection pin_dir = DIR_BETWEEN<IndexDirection>[idx(king_sq)][idx(from)];
            legal_mask &= cache.pin_masks[idx(pin_dir)];
        }
        Bitboard legal_quiet_pawn = legal_quiet & legal_mask;
        Bitboard legal_capture_pawn = legal_capture & legal_mask;

        // single push
        Bitboard bb_single_unoccupied = BB_SINGLE_PAWN_MOVES[idx(color)][idx(from)] & unoccupied;
        Bitboard bb_single = bb_single_unoccupied & legal_quiet_pawn;
        Bitboard bb_quiet = bb_single & ~BB_PROMOTION_RANKS;
        Bitboard bb_promotion = bb_single & BB_PROMOTION_RANKS;
        if (nonzero(bb_quiet)) {
            Square to = bitscan_forward(bb_quiet);
            moves.emplace_back(from, to, MoveType::QUIET);
        }
        if (nonzero(bb_promotion)) {
            Square to = bitscan_forward(bb_promotion);
            moves.emplace_back(from, to, MoveType::KNIGHT_PROMOTION);
            moves.emplace_back(from, to, MoveType::BISHOP_PROMOTION);
            moves.emplace_back(from, to, MoveType::ROOK_PROMOTION);
            moves.emplace_back(from, to, MoveType::QUEEN_PROMOTION);
        }

        // double pawn push
        if (nonzero(bb_single_unoccupied)) {
            Bitboard bb_double = BB_DOUBLE_PAWN_MOVES[idx(color)][idx(from)] & legal_quiet_pawn;
            if (nonzero(bb_double)) {
                Square to = bitscan_forward(bb_double);
                moves.emplace_back(from, to, MoveType::DOUBLE_PAWN_PUSH);
            }
        }

        // captures
        Bitboard bb_capture = bb_attacks<Piece::PAWN>(color, from) & legal_capture_pawn;
        Bitboard bb_capture_only = (bb_capture & ~BB_PROMOTION_RANKS);
        Bitboard bb_capture_promote = (bb_capture & BB_PROMOTION_RANKS);
        append_moves(moves, from, bb_capture_only, MoveType::CAPTURE);
        for (Square to : BBSquareScan(bb_capture_promote)) {
            moves.emplace_back(from, to, MoveType::KNIGHT_PROMOTION_CAPTURE);
            moves.emplace_back(from, to, MoveType::BISHOP_PROMOTION_CAPTURE);
            moves.emplace_back(from, to, MoveType::ROOK_PROMOTION_CAPTURE);
            moves.emplace_back(from, to, MoveType::QUEEN_PROMOTION_CAPTURE);
        }
    }

    if (board.ep_square != Square::NONE) {
        generate_legal_ep_moves(board, moves, cache);
    }
}

inline void generate_legal_castle_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Bitboard occupied = board.occupied;
    CastlingRights castling_rights = board.castling_rights;
    Bitboard bb_king = bb_square(board.king_sq[idx(color)]);
    if (nonzero(cache.opponent_attacks & bb_king)) {
        return;
    }

    // b-file attacks dont prevent castle
    Bitboard occupied_or_attacked = occupied | (cache.opponent_attacks & ~bb_file(File::B));

    if (castling_allowed<Piece::KING>(castling_rights, color) && zero(BB_CASTLE_PATHS<Piece::KING>[idx(color)] & occupied_or_attacked)) {
        moves.emplace_back(CASTLE_MOVES<Piece::KING>[idx(color)]);
    }
    if (castling_allowed<Piece::QUEEN>(castling_rights, color) && zero(BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)] & occupied_or_attacked)) {
        moves.emplace_back(CASTLE_MOVES<Piece::QUEEN>[idx(color)]);
    }
}


inline Bitboard generate_knight_attacks(const Board& board, Color color) {
    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    Bitboard bb_knight_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_knights)) {
        bb_knight_attacks |= bb_attacks<Piece::KNIGHT>(from);
    }
    return bb_knight_attacks;
}

template<Piece move_type> requires is_bishop_or_rook<move_type>
inline Bitboard generate_slider_attacks(const Board& board, Color color, Bitboard bb_occupied) {
    Bitboard bb_pieces = (board.pieces[idx(color)][idx(move_type)] | board.pieces[idx(color)][idx(Piece::QUEEN)]);
    Bitboard bb_slider_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pieces)) {
        bb_slider_attacks |= bb_attacks<move_type>(from, bb_occupied);
    }
    return bb_slider_attacks;
}

inline Bitboard generate_pawn_attacks(const Board& board, Color color) {
    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];
    Bitboard bb_pawn_attacks = Bitboard::EMPTY;
    for (Square from : BBSquareScan(bb_pawns)) {
        bb_pawn_attacks |= bb_attacks<Piece::PAWN>(color, from);
    }
    return bb_pawn_attacks;
}

inline Bitboard generate_king_attacks(const Board& board, Color color) {
    return bb_attacks<Piece::KING>(board.king_sq[idx(color)]);
}

template <bool omit_king=false>
inline Bitboard calculate_opponent_attacks(const Board& board) {
    Bitboard attacks = Bitboard::EMPTY;

    Color color = board.side_to_move;
    Color opponent_color = ~color;
    Bitboard occupied = board.occupied;
    
    if constexpr (omit_king) {
        occupied &= (~board.pieces[idx(color)][idx(Piece::KING)]);
    }

    attacks |= generate_knight_attacks(board, opponent_color);
    attacks |= generate_slider_attacks<Piece::ROOK>(board, opponent_color, occupied);
    attacks |= generate_slider_attacks<Piece::BISHOP>(board, opponent_color, occupied);
    attacks |= generate_pawn_attacks(board, opponent_color);
    attacks |= generate_king_attacks(board, opponent_color);

    return attacks;
}

template<bool test_attacks=false>
inline Bitboard calculate_checkers(const Board& board, Bitboard opponent_attacks = Bitboard::EMPTY) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;
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
    Bitboard pawn_checkers = bb_attacks<Piece::PAWN>(color, king_square) & opponent_pawns;
    Bitboard rook_checkers = bb_attacks<Piece::ROOK>(king_square, board.occupied) & (opponent_rooks | opponent_queens);
    Bitboard bishop_checkers = bb_attacks<Piece::BISHOP>(king_square, board.occupied) & (opponent_bishops | opponent_queens);

    return knight_checkers | pawn_checkers | rook_checkers | bishop_checkers;
}

template<Piece move_type>
requires is_bishop_or_rook<move_type>
Bitboard calculate_pinned_pieces(const Board& board, Bitboard pin_masks[num_of<IndexDirection>], Bitboard& block_check) {
    Bitboard pinned = Bitboard::EMPTY;

    Square king_square = board.king_sq[idx(board.side_to_move)];
    Color color = board.side_to_move;
    Color opponent_color = ~color;

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