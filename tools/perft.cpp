#include <format>
#include <print>
#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <future>
#include <bears_chess.hpp>

using namespace std;
using namespace bears_chess;

int main()
{
    init();

    print("{}", run_perft(load_fen(INITIAL_POSITION_FEN), 7));
    print("{}", run_perft(load_fen(PERFT_POSITION_2_FEN), 6));
    print("{}", run_perft(load_fen(PERFT_POSITION_3_FEN), 8));
    print("{}", run_perft(load_fen(PERFT_POSITION_4_FEN), 6));
    print("{}", run_perft(load_fen(PERFT_POSITION_5_FEN), 5));
    print("{}", run_perft(load_fen(PERFT_POSITION_6_FEN), 6));

    // print("{}", run_perft<LegalPolicy>(load_fen(INITIAL_POSITION_FEN), 6));
    // print("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_2_FEN), 5));
    // print("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_3_FEN), 7));
    // print("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_4_FEN), 5));
    // print("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_5_FEN), 4));
    // print("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_6_FEN), 5));

    return 0;
}
