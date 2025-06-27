#include <iostream>
#include "board_utils.hpp"

using namespace std;
using namespace bears_chess;

int main()
{
    std::cout << set_board_format(BoardFormat::HIDE_FEN);

    std::cout << load_fen(INITIAL_POSITION_FEN) << std::endl;
    std::cout << load_fen(PERFT_POSITION_2_FEN) << std::endl;
    std::cout << load_fen(PERFT_POSITION_3_FEN) << std::endl;
    std::cout << load_fen(PERFT_POSITION_4_FEN) << std::endl;
    std::cout << load_fen(PERFT_POSITION_5_FEN) << std::endl;
    std::cout << load_fen(PERFT_POSITION_6_FEN) << std::endl;

    for (Square s : iter<Square>) {
        cout << "Square: " << s << std::endl;
        // cout << set_bitboard_piece(Piece::NONE) << BB_KNIGHT_MOVES[idx(s)];
        cout << BB_KNIGHT_MOVES[idx(s)];
    }

    return 0;
}
