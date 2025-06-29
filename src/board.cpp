#include "board.hpp"
#include "bitboard.hpp"

namespace bears_chess {

Board::Board() : 
    pieces{},
    side_to_move(Color::WHITE),
    fullmove_number(1),
    castling_rights(CastlingRights::ALL),
    ep_square(Square::NONE),
    halfmove_clock(0)
{
    // Rank 1
    place(Color::WHITE, Piece::ROOK, Square::A1);
    place(Color::WHITE, Piece::KNIGHT, Square::B1);
    place(Color::WHITE, Piece::BISHOP, Square::C1);
    place(Color::WHITE, Piece::QUEEN, Square::D1);
    place(Color::WHITE, Piece::KING, Square::E1);
    place(Color::WHITE, Piece::BISHOP, Square::F1);
    place(Color::WHITE, Piece::KNIGHT, Square::G1);
    place(Color::WHITE, Piece::ROOK, Square::H1);

    // Rank 2
    place(Color::WHITE, Piece::PAWN, Square::A2);
    place(Color::WHITE, Piece::PAWN, Square::B2);
    place(Color::WHITE, Piece::PAWN, Square::C2);
    place(Color::WHITE, Piece::PAWN, Square::D2);
    place(Color::WHITE, Piece::PAWN, Square::E2);
    place(Color::WHITE, Piece::PAWN, Square::F2);
    place(Color::WHITE, Piece::PAWN, Square::G2);
    place(Color::WHITE, Piece::PAWN, Square::H2);

    // Rank 7
    place(Color::BLACK, Piece::PAWN, Square::A7);
    place(Color::BLACK, Piece::PAWN, Square::B7);
    place(Color::BLACK, Piece::PAWN, Square::C7);
    place(Color::BLACK, Piece::PAWN, Square::D7);
    place(Color::BLACK, Piece::PAWN, Square::E7);
    place(Color::BLACK, Piece::PAWN, Square::F7);
    place(Color::BLACK, Piece::PAWN, Square::G7);
    place(Color::BLACK, Piece::PAWN, Square::H7);
    
    // Rank 8
    place(Color::BLACK, Piece::ROOK, Square::A8);
    place(Color::BLACK, Piece::KNIGHT, Square::B8);
    place(Color::BLACK, Piece::BISHOP, Square::C8);
    place(Color::BLACK, Piece::QUEEN, Square::D8);
    place(Color::BLACK, Piece::KING, Square::E8);
    place(Color::BLACK, Piece::BISHOP, Square::F8);
    place(Color::BLACK, Piece::KNIGHT, Square::G8);
    place(Color::BLACK, Piece::ROOK, Square::H8);
}

UndoInfo Board::do_move(const Move &move)
{
    UndoInfo undo{
        move,
        Piece::NONE,
        castling_rights,
        ep_square,
        halfmove_clock
    };

    Color opponent = ~side_to_move;
    Square from = move.from;
    Square to = move.to;
    Piece moving_piece = get_piece_at(from);

    halfmove_clock++;

    if (is_capture(move.move_type)) {
        if (move.move_type == MoveType::EP_CAPTURE) {
            Square ep_captured = square_of(file_of(ep_square), rank_of(from));
            undo.captured = Piece::PAWN;
            remove(opponent, Piece::PAWN, ep_captured);
            ep_square = Square::NONE;
        } else {
            undo.captured = get_piece_of_color_at(opponent, to);
            remove(opponent, undo.captured, to);
        }
        halfmove_clock = 0;
    } else if (moving_piece == Piece::PAWN) {
        halfmove_clock = 0;
    }

    remove(side_to_move, moving_piece, from);

    if (is_promotion(move.move_type)) {
        moving_piece = promote_to(move.move_type);
    }

    place(side_to_move, moving_piece, to);

    if (move.move_type == MoveType::KING_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(side_to_move)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::KING>[idx(side_to_move)];
        remove(side_to_move, Piece::ROOK, rook_from);
        place(side_to_move, Piece::ROOK, rook_to);
        castling_rights = clear_castling_rights(castling_rights, side_to_move);
    } else if (move.move_type == MoveType::QUEEN_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(side_to_move)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::QUEEN>[idx(side_to_move)];
        remove(side_to_move, Piece::ROOK, rook_from);
        place(side_to_move, Piece::ROOK, rook_to);
        castling_rights = clear_castling_rights(castling_rights, side_to_move);
    } else if (moving_piece == Piece::KING) {
        castling_rights = clear_castling_rights(castling_rights, side_to_move);
    } else if (moving_piece == Piece::ROOK) {
        if (from == CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(side_to_move)]) {
            castling_rights = clear_half_castling_rights<Piece::KING>(castling_rights, side_to_move);
        } else if (from == CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(side_to_move)]) {
            castling_rights = clear_half_castling_rights<Piece::QUEEN>(castling_rights, side_to_move);
        }
    } else if (undo.captured == Piece::ROOK) {
        if (to == CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(opponent)]) {
            castling_rights = clear_half_castling_rights<Piece::KING>(castling_rights, opponent);
        } else if (to == CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(opponent)]) {
            castling_rights = clear_half_castling_rights<Piece::QUEEN>(castling_rights, opponent);
        }
    }

    fullmove_number += static_cast<int>(side_to_move);
    side_to_move = opponent;

    return undo;
}

void Board::undo_move(const UndoInfo &undo_info)
{
}

bool Board::is_legal(const Move &last_move) const
{
    // TODO
    return false;
}

} // namespace bears_chess
