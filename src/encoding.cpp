#include "bears_chess/mcts/encoding.hpp"
#include "bears_chess/bitboard.hpp"

#include <algorithm>

 namespace bears_chess {

static void fill_plane(InputPlanes& planes, size_t plane, float value) {
    std::ranges::fill(planes.plane(plane), value);
}

static void encode_castling(
    InputPlanes& planes,
    size_t king_plane,
    size_t queen_plane,
    CastlingRights rights,
    Color color
) {
    const bool kingside = color == Color::WHITE
        ? check_flag(rights, CastlingRights::WHITE_KING)
        : check_flag(rights, CastlingRights::BLACK_KING);
    const bool queenside = color == Color::WHITE
        ? check_flag(rights, CastlingRights::WHITE_QUEEN)
        : check_flag(rights, CastlingRights::BLACK_QUEEN);
    fill_plane(planes, king_plane, kingside ? 1.0f : 0.0f);
    fill_plane(planes, queen_plane, queenside ? 1.0f : 0.0f);
}

 InputPlanes encode_board(const Board& board) {
    InputPlanes planes;
    const Color color = board.side_to_move;
    const Color opponent_color = ~color;

    for (Piece piece : iter<Piece>) {
        for (Square square : BBSquareScan(board.pieces[idx(color)][idx(piece)])) {
            planes.at(InputPlanes::OWN_PIECES + idx(piece), canonical_square(square, color)) = 1.0f;
        }
        for (Square square : BBSquareScan(board.pieces[idx(opponent_color)][idx(piece)])) {
            planes.at(InputPlanes::OPPONENT_PIECES + idx(piece), canonical_square(square, color)) = 1.0f;
        }
    }

    encode_castling(
        planes, InputPlanes::OWN_CASTLE_KING, InputPlanes::OWN_CASTLE_QUEEN,
        board.castling_rights, color
    );
    encode_castling(
        planes, InputPlanes::OPPONENT_CASTLE_KING, InputPlanes::OPPONENT_CASTLE_QUEEN,
        board.castling_rights, opponent_color
    );

    if (board.ep_square != Square::NONE) {
        planes.at(InputPlanes::EN_PASSANT, canonical_square(board.ep_square, color)) = 1.0f;
    }

    fill_plane(planes, InputPlanes::HALFMOVE_CLOCK, std::min(board.halfmove_clock / 100.0f, 1.0f));

    return planes;
}

};