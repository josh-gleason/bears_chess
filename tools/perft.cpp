#include <bears_chess.hpp>

using namespace bears_chess;

using log::print, log::println;

int main()
{
    init();

    println("{}", run_perft(load_fen(INITIAL_POSITION_FEN), 7));
    println("{}", run_perft(load_fen(PERFT_POSITION_2_FEN), 6));
    println("{}", run_perft(load_fen(PERFT_POSITION_3_FEN), 8));
    println("{}", run_perft(load_fen(PERFT_POSITION_4_FEN), 6));
    println("{}", run_perft(load_fen(PERFT_POSITION_5_FEN), 5));
    println("{}", run_perft(load_fen(PERFT_POSITION_6_FEN), 6));

    // println("{}", run_perft<LegalPolicy>(load_fen(INITIAL_POSITION_FEN), 6));
    // println("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_2_FEN), 5));
    // println("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_3_FEN), 7));
    // println("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_4_FEN), 5));
    // println("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_5_FEN), 4));
    // println("{}", run_perft<LegalPolicy>(load_fen(PERFT_POSITION_6_FEN), 5));

    return 0;
}
