#include <random>
#include <cstdint>
#include "zobrist.hpp"
#include "types.hpp"
#include "board.hpp"

namespace bears_chess {

const uint64_t ZOBRIST_SEED = 42;
static ZobristHash ZOBRIST_PIECES[num_of<Color>][num_of<Piece>][num_of<Square>];
static ZobristHash ZOBRIST_BLACK_TO_MOVE;
static ZobristHash ZOBRIST_CASTLING_RIGHTS[num_of<CastlingRights>];
static ZobristHash ZOBRIST_EP_FILE[num_of<File>];

void init_zobrist() {
    std::mt19937_64 rng(ZOBRIST_SEED);
    for (Color color : iter<Color>) {
        for (Piece piece : iter<Piece>) {
            for (Square square : iter<Square>) {
                ZOBRIST_PIECES[idx(color)][idx(piece)][idx(square)] = static_cast<ZobristHash>(rng());
            }
        }
    }
    ZOBRIST_BLACK_TO_MOVE = static_cast<ZobristHash>(rng());
    for (CastlingRights castling_rights : iter<CastlingRights>) {
        ZOBRIST_CASTLING_RIGHTS[idx(castling_rights)] = static_cast<ZobristHash>(rng());
    }
    for (File ep_file : iter<File>) {
        ZOBRIST_EP_FILE[idx(ep_file)] = static_cast<ZobristHash>(rng());
    }
}

ZobristHash zobrist_piece(Color c, Piece p, Square s) {
    return ZOBRIST_PIECES[idx(c)][idx(p)][idx(s)];
}

ZobristHash zobrist_castling_rights(CastlingRights r) {
    return ZOBRIST_CASTLING_RIGHTS[idx(r)];
}

ZobristHash zobrist_side_to_move(Color c) {
    return (c == Color::WHITE) ? ZobristHash::ZERO : ZOBRIST_BLACK_TO_MOVE;
}

ZobristHash zobrist_toggle_side_to_move() {
    return ZOBRIST_BLACK_TO_MOVE;
}

ZobristHash zobrist_ep_file(File f) {
    return ZOBRIST_EP_FILE[idx(f)];
}

ZobristHash compute_zobrist_hash(const Board& board) {
    ZobristHash hash = ZobristHash::ZERO;
    for (Square square : BBSquareScan(board.occupied)) {
        Color color = board.last_color_sq[idx(square)];
        Piece piece = board.last_piece_sq[idx(square)];
        hash ^= zobrist_piece(color, piece, square);
    }
    hash ^= zobrist_castling_rights(board.castling_rights);
    hash ^= zobrist_side_to_move(board.side_to_move);
    hash ^= zobrist_ep_file(file_of(board.ep_square));
    return hash;
}

}  // bears_chess
