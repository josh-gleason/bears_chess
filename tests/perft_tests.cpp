#include "board.hpp"
#include "board_utils.hpp"
#include "perft_utils.hpp"
#include "movegen.hpp"

#include <gtest/gtest.h>

using namespace bears_chess;

TEST(PerftTest, StartPosDepth6) {
    EXPECT_EQ(run_perft(load_fen(INITIAL_POSITION_FEN), 6), 119060324);
}

TEST(PerftTest, KiwiPeteDepth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_2_FEN), 5), 193690690);
}

TEST(PerftTest, Pos3Depth7) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_3_FEN), 7), 178633661);
}

TEST(PerftTest, Pos4Depth6) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_4_FEN), 6), 706045033);
}

TEST(PerftTest, Pos5Depth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_5_FEN), 5), 89941194);
}

TEST(PerftTest, Pos6Depth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_6_FEN), 5), 164075551);
}

