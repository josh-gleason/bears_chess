#include "bears_chess.hpp"

#include <gtest/gtest.h>

using namespace bears_chess;

class PerftTest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

TEST_F(PerftTest, StartPosDepth6) {
    EXPECT_EQ(run_perft(load_fen(INITIAL_POSITION_FEN), 6).nodes, 119060324);
}

TEST_F(PerftTest, KiwiPeteDepth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_2_FEN), 5).nodes, 193690690);
}

TEST_F(PerftTest, Pos3Depth7) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_3_FEN), 7).nodes, 178633661);
}

TEST_F(PerftTest, Pos4Depth6) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_4_FEN), 6).nodes, 706045033);
}

TEST_F(PerftTest, Pos5Depth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_5_FEN), 5).nodes, 89941194);
}

TEST_F(PerftTest, Pos6Depth5) {
    EXPECT_EQ(run_perft(load_fen(PERFT_POSITION_6_FEN), 5).nodes, 164075551);
}

