#include "bears_chess.hpp"

#include <gtest/gtest.h>

using namespace bears_chess;
using MoveGenType::LEGAL, MoveGenType::PSEUDO_LEGAL;
using std::print, std::println;

class MakeUnmakeTest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

TEST_F(MakeUnmakeTest, Legal_KiwipeteDepth4) {
    EXPECT_TRUE(test_do_undo(load_fen(PERFT_POSITION_2_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, Pseudolegal_KiwipeteDepth4) {
    EXPECT_TRUE(test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_2_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, Legal_Pos3Depth4) {
    EXPECT_TRUE(test_do_undo(load_fen(PERFT_POSITION_3_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, PseudoLegal_Pos3Depth4) {
    EXPECT_TRUE(test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_3_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, Legal_Pos4Depth4) {
    EXPECT_TRUE(test_do_undo(load_fen(PERFT_POSITION_4_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, PseudoLegal_Pos4Depth4) {
    EXPECT_TRUE(test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_4_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, Legal_Pos5Depth4) {
    EXPECT_TRUE(test_do_undo(load_fen(PERFT_POSITION_5_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, PseudoLegal_Pos5Depth4) {
    EXPECT_TRUE(test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_5_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, Legal_Pos6Depth4) {
    EXPECT_TRUE(test_do_undo(load_fen(PERFT_POSITION_6_FEN), 4).empty());
}

TEST_F(MakeUnmakeTest, PseudoLegal_Pos6Depth4) {
    EXPECT_TRUE(test_do_undo<PSEUDO_LEGAL>(load_fen(PERFT_POSITION_6_FEN), 4).empty());
}
