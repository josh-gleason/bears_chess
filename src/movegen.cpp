#include "movegen.hpp"
#include "evaluation_utils.hpp"
#include "magics.hpp"
#include <algorithm>

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

inline void generate_knight_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = BB_KNIGHT_MOVES[idx(from)];
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
    Bitboard bb_moves = BB_KING_MOVES[idx(from)];
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
        Bitboard bb_capture_pattern = BB_CAPTURE_PAWN_MOVES[idx(color)][idx(from)];
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
        Bitboard bb_ep_pawn_mask = BB_CAPTURE_PAWN_MOVES[idx(~color)][idx(to)];
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
        Bitboard bb_moves = magic_lookup<move_type>(from, occupied);
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

template<bool check_only=false>
inline bool generate_legal_king_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(opponent_color)];

    Square from = board.king_sq[idx(color)];
    Bitboard bb_moves = BB_KING_MOVES[idx(from)];
    Bitboard bb_quiet = (bb_moves & unoccupied & ~cache.opponent_attacks);
    Bitboard bb_capture = (bb_moves & opponent_occupied & ~cache.opponent_attacks);
    if constexpr (check_only) if (nonzero(bb_quiet | bb_capture)) return true;
    append_moves(moves, from, bb_quiet, MoveType::QUIET);
    append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    
    return false;
}

template<bool check_only=false>
inline bool generate_legal_knight_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)] & ~cache.pinned_pieces;
    Bitboard legal_quiet = ~board.occupied & cache.block_mask;
    Bitboard legal_capture = board.occupied_by_color[idx(~color)] & cache.block_mask;

    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_moves = BB_KNIGHT_MOVES[idx(from)];
        Bitboard bb_quiet = (bb_moves & legal_quiet);
        Bitboard bb_capture = (bb_moves & legal_capture);
        if constexpr (check_only) if (nonzero(bb_quiet | bb_capture)) return true;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }
    return false;
}

template<Piece move_type, bool check_only=false> requires is_bishop_or_rook<move_type>
inline bool generate_legal_slider_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
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
        Bitboard bb_moves = magic_lookup<move_type>(from, occupied) & cache.pin_masks[idx(pin_dir)];
        Bitboard bb_quiet = bb_moves & legal_quiet;
        Bitboard bb_capture = bb_moves & legal_capture;
        if constexpr (check_only) if (nonzero(bb_quiet | bb_capture)) return true;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }

    for (Square from : BBSquareScan(unpinned_sliders)) {
        Bitboard bb_moves = magic_lookup<move_type>(from, occupied);
        Bitboard bb_quiet = bb_moves & legal_quiet;
        Bitboard bb_capture = bb_moves & legal_capture;
        if constexpr (check_only) if (nonzero(bb_quiet | bb_capture)) return true;
        append_moves(moves, from, bb_quiet, MoveType::QUIET);
        append_moves(moves, from, bb_capture, MoveType::CAPTURE);
    }

    return false;
}

template<bool check_only=false>
inline bool generate_legal_ep_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Color opponent_color = ~color;

    Square king_sq = board.king_sq[idx(color)];
    Bitboard bb_king = bb_square(king_sq);

    Square to = board.ep_square;
    Bitboard bb_opponent_pawn = BB_SINGLE_PAWN_MOVES[idx(~color)][idx(to)];
    if (nonzero(cache.block_mask & bb_opponent_pawn)) {
        // pawn is allowed to be captured
        Bitboard bb_ep_pawn_mask = BB_CAPTURE_PAWN_MOVES[idx(~color)][idx(to)];
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
                        Bitboard attack = magic_lookup<Piece::ROOK>(king_sq, board.occupied & ~(bb_opponent_pawn | bb_from));
                        exposing = nonzero(opponent_sliders & attack);
                    } else {
                        exposing = false;
                    }
                }
                if (!exposing) {
                    if constexpr (check_only) return true;
                    moves.emplace_back(from, to, MoveType::EP_CAPTURE);
                }
            }
        }
    }
    return false;
}

template<bool check_only=false>
inline bool generate_legal_pawn_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
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
            if constexpr (check_only) return true;
            Square to = bitscan_forward(bb_quiet);
            moves.emplace_back(from, to, MoveType::QUIET);
        }
        if (nonzero(bb_promotion)) {
            if constexpr (check_only) return true;
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
                if constexpr (check_only) return true;
                Square to = bitscan_forward(bb_double);
                moves.emplace_back(from, to, MoveType::DOUBLE_PAWN_PUSH);
            }
        }

        // captures
        Bitboard bb_capture = BB_CAPTURE_PAWN_MOVES[idx(color)][idx(from)] & legal_capture_pawn;
        if constexpr (check_only) if (nonzero(bb_capture)) return true;
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
        bool any_moves = generate_legal_ep_moves<check_only>(board, moves, cache);
        if constexpr (check_only) if (any_moves) return true;
    }
    return false;
}

template<bool check_only=false>
inline bool generate_legal_castle_moves(const Board& board, MoveList& moves, const BoardCache& cache) {
    Color color = board.side_to_move;
    Bitboard occupied = board.occupied;
    CastlingRights castling_rights = board.castling_rights;
    Bitboard bb_king = bb_square(board.king_sq[idx(color)]);
    if (nonzero(cache.opponent_attacks & bb_king)) {
        return false;
    }

    // b-file attacks dont prevent castle
    Bitboard occupied_or_attacked = occupied | (cache.opponent_attacks & ~bb_file(File::B));

    if (castling_allowed<Piece::KING>(castling_rights, color) && zero(BB_CASTLE_PATHS<Piece::KING>[idx(color)] & occupied_or_attacked)) {
        if constexpr (check_only) return true;
        moves.emplace_back(CASTLE_MOVES<Piece::KING>[idx(color)]);
    }
    if (castling_allowed<Piece::QUEEN>(castling_rights, color) && zero(BB_CASTLE_PATHS<Piece::QUEEN>[idx(color)] & occupied_or_attacked)) {
        if constexpr (check_only) return true;
        moves.emplace_back(CASTLE_MOVES<Piece::QUEEN>[idx(color)]);
    }
    return false;
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

bool is_checkmate(const Board& board) {
    MoveList unused;
    BoardCache cache;
    cache.opponent_attacks = calculate_opponent_attacks<true>(board);
    Bitboard checkers = calculate_checkers<true>(board, cache.opponent_attacks);
    int num_checkers = popcount(checkers);

    if (num_checkers == 2) {
        return !generate_legal_king_moves<true>(board, unused, cache);
    } else if (num_checkers == 1) {
        if (generate_legal_king_moves<true>(board, unused, cache))
            return false;
        if (generate_legal_castle_moves<true>(board, unused, cache))
            return false;
        cache.block_mask = checkers;
        cache.pinned_pieces = (
            calculate_pinned_pieces<Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
            | calculate_pinned_pieces<Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
        );
        if (generate_legal_knight_moves<true>(board, unused, cache))
            return false;
        if (generate_legal_slider_moves<Piece::ROOK, true>(board, unused, cache))
            return false;
        if (generate_legal_slider_moves<Piece::BISHOP, true>(board, unused, cache))
            return false;
        if (generate_legal_pawn_moves<true>(board, unused, cache))
            return false;
        return true;
    }
    return false;
}

} // namespace bears_chess
