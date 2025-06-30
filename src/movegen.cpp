#include "movegen.hpp"
#include "magics.hpp"

namespace bears_chess {

inline void generate_knight_moves(const Board& board, MoveList& moves) {
    // find knights for our color
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_move_pattern = BB_KNIGHT_MOVES[idx(from)];
        Bitboard bb_quiet = (bb_move_pattern & unoccupied);
        Bitboard bb_capture = (bb_move_pattern & opponent_occupied);
        for (Square to : BBSquareScan(bb_quiet)) {
            moves.emplace_back(Move{from, to, MoveType::QUIET});
        }
        for (Square to : BBSquareScan(bb_capture)) {
            moves.emplace_back(Move{from, to, MoveType::CAPTURE});
        }
    }
}

inline void generate_knight_moves_alt(const Board& board, MoveList& moves) {
    // TODO: test to see if this is faster
    Color color = board.side_to_move;

    Bitboard self_unoccupied = ~board.occupied_by_color[idx(color)];
    Bitboard occupied = board.occupied;

    Bitboard bb_knights = board.pieces[idx(color)][idx(Piece::KNIGHT)];
    for (Square from : BBSquareScan(bb_knights)) {
        Bitboard bb_move_pattern = BB_KNIGHT_MOVES[idx(from)];
        Bitboard bb_moves = (bb_move_pattern & self_unoccupied);
        for (Square to : BBSquareScan(bb_moves)) {
            bool capture = nonzero(bb_square(to) & occupied);
            MoveType move_type = capture ? MoveType::CAPTURE : MoveType::QUIET;
            moves.emplace_back(Move{from, to, move_type});
        }
    }
}

inline void generate_king_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard unoccupied = ~board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Square from = bitscan_forward(board.pieces[idx(color)][idx(Piece::KING)]);
    Bitboard bb_move_pattern = BB_KING_MOVES[idx(from)];

    Bitboard bb_quiet = (bb_move_pattern & unoccupied);
    Bitboard bb_capture = (bb_move_pattern & opponent_occupied);
    for (Square to : BBSquareScan(bb_quiet)) {
        moves.emplace_back(Move{from, to, MoveType::QUIET});
    }
    for (Square to : BBSquareScan(bb_capture)) {
        moves.emplace_back(Move{from, to, MoveType::CAPTURE});
    }
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
            moves.emplace_back(Move{from, to, MoveType::QUIET});
        }
        if (nonzero(bb_promotion)) {
            Square to = bitscan_forward(bb_promotion);
            moves.emplace_back(Move{from, to, MoveType::KNIGHT_PROMOTION});
            moves.emplace_back(Move{from, to, MoveType::BISHOP_PROMOTION});
            moves.emplace_back(Move{from, to, MoveType::ROOK_PROMOTION});
            moves.emplace_back(Move{from, to, MoveType::QUEEN_PROMOTION});
        }

        // double pawn push
        if (nonzero(bb_single)) {
            Bitboard bb_double = (BB_DOUBLE_PAWN_MOVES[idx(color)][idx(from)] & unoccupied);
            if (nonzero(bb_double)) {
                Square to = bitscan_forward(bb_double);
                moves.emplace_back(Move{from, to, MoveType::DOUBLE_PAWN_PUSH});
            }
        }

        // captures
        Bitboard bb_capture_pattern = BB_CAPTURE_PAWN_MOVES[idx(color)][idx(from)];
        Bitboard bb_capture = (bb_capture_pattern & opponent_occupied);
        Bitboard bb_capture_only = (bb_capture & ~BB_PROMOTION_RANKS);
        Bitboard bb_capture_promote = (bb_capture & BB_PROMOTION_RANKS);
        for (Square to : BBSquareScan(bb_capture_only)) {
            moves.emplace_back(Move{from, to, MoveType::CAPTURE});
        }
        for (Square to : BBSquareScan(bb_capture_promote)) {
            moves.emplace_back(Move{from, to, MoveType::KNIGHT_PROMOTION_CAPTURE});
            moves.emplace_back(Move{from, to, MoveType::BISHOP_PROMOTION_CAPTURE});
            moves.emplace_back(Move{from, to, MoveType::ROOK_PROMOTION_CAPTURE});
            moves.emplace_back(Move{from, to, MoveType::QUEEN_PROMOTION_CAPTURE});
        }
    }

    if (board.ep_square != Square::NONE) {
        Square to = board.ep_square;
        Bitboard bb_ep_pawn_mask = BB_CAPTURE_PAWN_MOVES[idx(~color)][idx(to)];
        Bitboard bb_capture_ep = bb_ep_pawn_mask & board.pieces[idx(color)][idx(Piece::PAWN)];
        for (Square from : BBSquareScan(bb_capture_ep)) {
            moves.emplace_back(Move{from, to, MoveType::EP_CAPTURE});
        }
    }
}

template <Piece target_piece, Piece move_type>
inline void generate_slider_moves(const Board& board, MoveList& moves) {
    Color color = board.side_to_move;

    Bitboard occupied = board.occupied;
    Bitboard opponent_occupied = board.occupied_by_color[idx(~color)];

    Bitboard bb_pieces = board.pieces[idx(color)][idx(target_piece)];
    for (Square from : BBSquareScan(bb_pieces)) {
        Bitboard bb_moves = magic_lookup<move_type>(from, occupied);
        Bitboard bb_quiet = bb_moves & ~occupied;
        Bitboard bb_capture = bb_moves & opponent_occupied;

        for (Square to : BBSquareScan(bb_quiet)) {
            moves.emplace_back(Move{from, to, MoveType::QUIET});
        }
        for (Square to : BBSquareScan(bb_capture)) {
            moves.emplace_back(Move{from, to, MoveType::CAPTURE});
        }
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
    // TODO: Not thread safe, should return an iterator tbh
    static MoveList moves;
    moves.resize(0);

    generate_knight_moves(board, moves);
    generate_king_moves(board, moves);
    generate_pawn_moves(board, moves);
    generate_slider_moves<Piece::ROOK, Piece::ROOK>(board, moves);
    generate_slider_moves<Piece::QUEEN, Piece::ROOK>(board, moves);
    generate_slider_moves<Piece::BISHOP, Piece::BISHOP>(board, moves);
    generate_slider_moves<Piece::QUEEN, Piece::BISHOP>(board, moves);
    generate_castle_moves(board, moves);

    return moves;
}

} // namespace bears_chess
