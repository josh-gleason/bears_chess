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
    return UndoInfo();
}

void Board::undo_move(const UndoInfo &undo_info)
{
}

} // namespace bears_chess
