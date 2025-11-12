#include "types.hpp"

namespace bears_chess {

class Board;

void init_zobrist();

ZobristHash compute_zobrist_hash(const Board& board);
ZobristHash zobrist_piece(Color c, Piece p, Square s);
ZobristHash zobrist_castling_rights(CastlingRights r);
ZobristHash zobrist_side_to_move(Color c);
ZobristHash zobrist_toggle_side_to_move();
ZobristHash zobrist_ep_file(File f);

}   // bears_chess
