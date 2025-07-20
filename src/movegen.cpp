#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "movegen_helpers.hpp"

namespace bears_chess {

inline void generate_knight_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = bb_attacks<Piece::KNIGHT>(from);
        Bitboard bb_quiet = (bb_moves & unoccupied);
        Bitboard bb_capture = (bb_moves & opponent_occupied);
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
}

inline void generate_king_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = bb_attacks<Piece::KING>(from);
    Bitboard bb_quiet = (bb_moves & unoccupied);
    Bitboard bb_capture = (bb_moves & opponent_occupied);
    append_moves(moves, from, bb_quiet, MoveType::QUIET);
    append_moves(moves, from, bb_capture, MoveType::CAPTURE);
}

inline void generate_pawn_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_pawns = board.pieces[idx(color)][idx(Piece::PAWN)];

    for (Square from : BBSquareScan(bb_pawns)) {
        // single pawn push
        Bitboard bb_single = (BB_SINGLE_PAWN_MOVES[idx(color)][idx(from)] & unoccupied);
        Bitboard bb_quiet = (bb_single & ~BB_PROMOTION_RANKS);
        Bitboard bb_promotion = (bb_single & BB_PROMOTION_RANKS);

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
        if (nonzero(bb_single)) {
            Bitboard bb_double = (BB_DOUBLE_PAWN_MOVES[idx(color)][idx(from)] & unoccupied);
            if (nonzero(bb_double)) {
                Square to = bitscan_forward(bb_double);
                moves.emplace_back(from, to, MoveType::DOUBLE_PAWN_PUSH);
            }
        }

        // captures
        Bitboard bb_capture_pattern = bb_attacks<Piece::PAWN>(color, from);
        Bitboard bb_capture = (bb_capture_pattern & opponent_occupied);
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
        Square to = board.ep_square;
        Bitboard bb_ep_pawn_mask = bb_attacks<Piece::PAWN>(~color, to);
        Bitboard bb_capture_ep = bb_ep_pawn_mask & board.pieces[idx(color)][idx(Piece::PAWN)];
        for (Square from : BBSquareScan(bb_capture_ep)) {
            moves.emplace_back(from, to, MoveType::EP_CAPTURE);
        }
    }
}

template <Piece move_type> requires is_bishop_or_rook<move_type>
void generate_slider_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard occupied = board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_pieces = board.pieces[idx(color)][idx(move_type)] | board.pieces[idx(color)][idx(Piece::QUEEN)];
    for (Square from : BBSquareScan(bb_pieces)) {
        Bitboard bb_moves = bb_attacks<move_type>(from, occupied);
        Bitboard bb_quiet = bb_moves & ~occupied;
        Bitboard bb_capture = bb_moves & opponent_occupied;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
}

inline void generate_castle_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;
    Bitboard occupied = board.occupied;
    CastlingRights castling_rights = board.castling_rights;
    if (castling_allowed<Piece::KING>(castling_rights, color) && !(occupied & BB_CASTLE_PATHS<Piece::KING>[idx(color)])) {
        moves.emplace_back(CASTLE_MOVES<Piece::KING>[idx(color)]);
    }
    if (castling_allowed<Piece::QUEEN>(castling_rights, color) && !(occupied & BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)])) {
        moves.emplace_back(CASTLE_MOVES<Piece::QUEEN>[idx(color)]);
    }
}

MoveList generate_pseudo_legal_moves(const Board& board) {
    MoveList moves;
    moves.reserve(218);

    generate_knight_moves(board, moves);
    generate_king_moves(board, moves);
    generate_pawn_moves(board, moves);
    generate_slider_moves<Piece::ROOK>(board, moves);
    generate_slider_moves<Piece::BISHOP>(board, moves);
    generate_castle_moves(board, moves);

    return moves;
}

MoveList generate_legal_moves(const Board& board) {
    BoardCache cache;
    MoveList moves;
    moves.reserve(218);

    cache.opponent_attacks = calculate_opponent_attacks<true>(board);
    Bitboard checkers = calculate_checkers<true>(board, cache.opponent_attacks);
    int num_checkers = popcount(checkers);

    if (num_checkers == 2) {
        generate_legal_king_moves(board, moves, cache);
    } else {
        // hold mask of squares we can move pieces to to block check
        cache.block_mask = num_checkers == 0 ? Bitboard::FULL : checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );

        generate_legal_king_moves(board, moves, cache);
        generate_legal_knight_moves(board, moves, cache);
        generate_legal_pawn_moves(board, moves, cache);
        generate_legal_slider_moves<Piece::ROOK>(board, moves, cache);
        generate_legal_slider_moves<Piece::BISHOP>(board, moves, cache);
        generate_legal_castle_moves(board, moves, cache);
    }

    return moves;
}

} // namespace bears_chess