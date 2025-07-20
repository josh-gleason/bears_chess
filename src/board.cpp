#include "board.hpp"
#include "bitboard.hpp"

namespace bears_chess {

Board::Board() : 
    pieces{},
    side_to_move(Color::WHITE),
    fullmove_number(1),
    castling_rights(CastlingRights::ALL),
    ep_square(Square::NONE),
    halfmove_clock(0),
    occupied(Bitboard::EMPTY),
    occupied_by_color{Bitboard::EMPTY, Bitboard::EMPTY}
{
    for (Color c : iter<Color>)
        for (Piece p : iter<Piece>)
            pieces[idx(c)][idx(p)] = Bitboard::EMPTY;

    // Rank 1
    place(Color::WHITE, Piece::ROOK, Square::A1);
    place(Color::WHITE, Piece::KNIGHT, Square::B1);
    place(Color::WHITE, Piece::BISHOP, Square::C1);
    place(Color::WHITE, Piece::QUEEN, Square::D1);
    place_king(Color::WHITE, Square::E1);
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
    place_king(Color::BLACK, Square::E8);
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

    const Color us = side_to_move;
    const Color them = ~side_to_move;
    const Square from = move.from;
    const Square to = move.to;

    Piece moving_piece = get_piece_of_color_at(us, from);

    ep_square = Square::NONE;

    halfmove_clock++;

    if (is_capture(move.move_type)) {
        halfmove_clock = 0;
        if (move.move_type == MoveType::EP_CAPTURE) {
            undo.captured = Piece::PAWN;
            Square captured_sq = captured_ep_square(us, to);
            remove(them, Piece::PAWN, captured_sq);
        } else {
            undo.captured = get_piece_of_color_at(them, to);
            remove(them, undo.captured, to);
        }
    } else if (moving_piece == Piece::PAWN) {
        halfmove_clock = 0;
        if (move.move_type == MoveType::DOUBLE_PAWN_PUSH) {
            ep_square = double_push_ep_square(us, to);
        }
    }

    remove(us, moving_piece, from);

    if (is_promotion(move.move_type)) {
        moving_piece = promote_to(move.move_type);
    }

    place(us, moving_piece, to);

    if (move.move_type == MoveType::KING_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(us)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::KING>[idx(us)];
        remove(us, Piece::ROOK, rook_from);
        place(us, Piece::ROOK, rook_to);
        castling_rights = clear_castling_rights(castling_rights, us);
        king_sq[idx(us)] = to;
    } else if (move.move_type == MoveType::QUEEN_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(us)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::QUEEN>[idx(us)];
        remove(us, Piece::ROOK, rook_from);
        place(us, Piece::ROOK, rook_to);
        castling_rights = clear_castling_rights(castling_rights, us);
        king_sq[idx(us)] = to;
    } else if (moving_piece == Piece::KING) {
        castling_rights = clear_castling_rights(castling_rights, us);
        king_sq[idx(us)] = to;
    } else if (moving_piece == Piece::ROOK) {
        if (from == CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(us)]) {
            castling_rights = clear_half_castling_rights<Piece::KING>(castling_rights, us);
        } else if (from == CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(us)]) {
            castling_rights = clear_half_castling_rights<Piece::QUEEN>(castling_rights, us);
        }
    }
    
    if (undo.captured == Piece::ROOK) {
        if (to == CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(them)]) {
            castling_rights = clear_half_castling_rights<Piece::KING>(castling_rights, them);
        } else if (to == CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(them)]) {
            castling_rights = clear_half_castling_rights<Piece::QUEEN>(castling_rights, them);
        }
    }

    fullmove_number += static_cast<int>(side_to_move);
    side_to_move = them;

    return undo;
}

void Board::undo_move(const UndoInfo &undo_info)
{
    const Move& move = undo_info.move;
    const Square from = move.from;
    const Square to = move.to;
    const Color us = ~side_to_move;
    const Color them = side_to_move;

    fullmove_number -= idx(us);
    side_to_move = us;
    halfmove_clock = undo_info.halfmove_clock;
    castling_rights = undo_info.castling_rights;
    ep_square = undo_info.ep_square;

    Piece moving_piece = get_piece_of_color_at(us, to);
    remove(us, moving_piece, to);
    if (is_promotion(move.move_type)) {
        place(us, Piece::PAWN, from);
    } else if (moving_piece == Piece::KING) {
        place_king(us, from);
    } else {
        place(us, moving_piece, from);
    }

    if (is_capture(move.move_type)) {   // undo_info.captured_piece != Piece::NONE may be faster?
        if (move.move_type == MoveType::EP_CAPTURE) {
            Square captured_sq = captured_ep_square(us, to);
            place(them, Piece::PAWN, captured_sq);
        } else {
            place(them, undo_info.captured, to);
        }
    } else if (move.move_type == MoveType::KING_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::KING>[idx(us)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::KING>[idx(us)];
        remove(us, Piece::ROOK, rook_to);
        place(us, Piece::ROOK, rook_from);
    } else if (move.move_type == MoveType::QUEEN_CASTLE) {
        Square rook_from = CASTLE_ROOK_FROM_SQUARES<Piece::QUEEN>[idx(us)];
        Square rook_to = CASTLE_ROOK_TO_SQUARES<Piece::QUEEN>[idx(us)];
        remove(us, Piece::ROOK, rook_to);
        place(us, Piece::ROOK, rook_from);
    }
}

bool Board::is_legal(const Move &last_move) const
{
    Color us = ~side_to_move;
    Color them = side_to_move;
    Square king_square = king_sq[idx(us)];
    if (is_square_attacked(king_square, them))
        return false;
    if (last_move.move_type == MoveType::KING_CASTLE)
        return !(
            is_square_attacked(sq_shift<1>(king_square, Direction::WEST), them) ||
            is_square_attacked(sq_shift<2>(king_square, Direction::WEST), them)
        );
    if (last_move.move_type == MoveType::QUEEN_CASTLE)
        return !(
            is_square_attacked(sq_shift<1>(king_square, Direction::EAST), them) ||
            is_square_attacked(sq_shift<2>(king_square, Direction::EAST), them)
        );
    return true;
}

} // namespace bears_chess
