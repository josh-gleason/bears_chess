#include <format>
#include <print>
#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <future>
#include "bears_chess.hpp"

using namespace std;
using namespace bears_chess;
using MoveGenType::LEGAL, MoveGenType::PSEUDO_LEGAL;


template<MoveGenType move_gen_type>
void show_do_undo_failures(const Board& board, int max_depth) {
    auto failure_moves = test_do_undo<move_gen_type>(board, max_depth);
    
    if (!failure_moves.empty()) {
        Board temp_board = board;
        println("Failure");
        print("Starting fen: {:F}", board);
        for (auto it = failure_moves.rbegin(); it != failure_moves.rend(); ++it) {
            Move move = *it;
            println("    {}. {}", std::distance(failure_moves.rbegin(), it) + 1, move);
            println("{}", show_highlights(temp_board, bb_square(move.from) | bb_square(move.to)));
            temp_board.do_move(move);
        }
    }
}

int main()
{
    init();

    // show_do_undo_failures<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_6_FEN), 6);
    // show_moves<PSEUDO_LEGAL>(board);
    // test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_2_FEN), 5);

    print("{}", run_perft(load_fen(INITIAL_POSITION_FEN), 7));
    print("{}", run_perft(load_fen(PERFT_POSITION_2_FEN), 6));
    print("{}", run_perft(load_fen(PERFT_POSITION_3_FEN), 8));
    print("{}", run_perft(load_fen(PERFT_POSITION_4_FEN), 6));
    print("{}", run_perft(load_fen(PERFT_POSITION_5_FEN), 5));
    print("{}", run_perft(load_fen(PERFT_POSITION_6_FEN), 6));

    // print("{}", run_perft<LEGAL, true>(load_fen(INITIAL_POSITION_FEN), 6));
    // print("{}", run_perft<LEGAL, true>(load_fen(PERFT_POSITION_2_FEN), 6));
    // print("{}", run_perft<LEGAL, true>(load_fen(PERFT_POSITION_3_FEN), 8));

    return 0;
}
