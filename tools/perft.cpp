#include <iostream>
#include "board_utils.hpp"
#include "movegen.hpp"

using namespace std;
using namespace bears_chess;

int main()
{
    // std::cout << load_fen(INITIAL_POSITION_FEN) << std::endl;
    // std::cout << load_fen(PERFT_POSITION_2_FEN) << std::endl;
    // std::cout << load_fen(PERFT_POSITION_3_FEN) << std::endl;
    // std::cout << load_fen(PERFT_POSITION_4_FEN) << std::endl;
    // std::cout << load_fen(PERFT_POSITION_5_FEN) << std::endl;
    // std::cout << load_fen(PERFT_POSITION_6_FEN) << std::endl;

    Board board = load_fen(PERFT_POSITION_2_FEN);

    std::cout << set_board_format((board.side_to_move == Color::WHITE ? BoardFormat::NONE : BoardFormat::ORIENT_BLACK) | BoardFormat::HIDE_FEN);

    MoveList moves = generate_pseudo_legal_moves(board);

    std::cout << moves.size() << std::endl;

    for (Square from : BBSquareScan(board.occupied_by_color[idx(board.side_to_move)])) {
        Bitboard highlights = bb_square(from);
        for (int i = moves.size() - 1; i >= 0; --i) {
            if (moves[i].from == from) {
                std::cout << moves[i] << std::endl;
                highlights |= bb_square(moves[i].to);
                moves.erase(moves.begin() + i);
            }
        }
        std::cout << show_highlights(board, highlights);
    }

    if (moves.size() > 0) {
        std::cout << "ERROR: Should Be empty!!!!" << std::endl;
        for (auto move : moves) {
            std::cout << move << std::endl;
        }
    }

    return 0;
}
