#include "bears_chess.hpp"

#include <gtest/gtest.h>

using namespace bears_chess;

class SEETest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

static int16_t compute_see(const std::string& fen, const std::string uci_move) {
    Board board = load_fen(fen);
    Move move = convert_uci_to_move(uci_move, board);
    if (board.side_to_move == Color::WHITE) {
        return static_exchange_evaluation<Color::WHITE>(board, move);
    }
    return static_exchange_evaluation<Color::BLACK>(board, move);    
}

TEST_F(SEETest, PawnTakesUndefendedPawn) {
    EXPECT_EQ(compute_see("k7/8/8/3p4/2P5/8/8/K7 w - - 0 1", "c4d5"), 100);
}

TEST_F(SEETest, PawnTakesDefendedPawn) {
    EXPECT_EQ(compute_see("k7/8/4p3/3p4/2P5/8/8/K7 w - - 0 1", "c4d5"), 0);
}

TEST_F(SEETest, RookTakesPawnDefByPawn) {
    EXPECT_EQ(compute_see("k7/2p5/3p4/8/8/8/3R4/K7 w - - 0 1", "d2d6"), -400);
}

TEST_F(SEETest, QueenTakesUndefendedRook) {
    EXPECT_EQ(compute_see("k7/8/8/3r4/8/8/8/K2Q4 w - - 0 1", "d1d5"), 500);
}

TEST_F(SEETest, QueenTakesRookDefByPawn) {
    EXPECT_EQ(compute_see("k7/8/4p3/3r4/8/8/8/K2Q4 w - - 0 1", "d1d5"), -400);
}

TEST_F(SEETest, RookTakesPawnWithRookXRayBehind) {
    EXPECT_EQ(compute_see("k7/8/4p3/3p4/8/8/3R4/K2R4 w - - 0 1", "d2d5"), -300);
}

TEST_F(SEETest, RookTakesKnightWithRookXRayBehind) {
    EXPECT_EQ(compute_see("k7/8/4p3/3n4/8/8/3R4/K2R4 w - - 0 1", "d2d5"), -100);
}

TEST_F(SEETest, KingMayNotJoinDefendedSquare) {
    EXPECT_EQ(compute_see("k7/8/4pn2/2Kp4/2P5/8/8/8 w - - 0 1", "c4d5"), 0);
}

TEST_F(SEETest, KingTakesUndefendedPawn) {
    EXPECT_EQ(compute_see("k7/8/8/8/8/2p5/2K5/8 w - - 0 1", "c2c3"), 100);
}

TEST_F(SEETest, EnPassantCaptureWithRookXRayBehind) {
    EXPECT_EQ(compute_see("3r3k/8/8/3pP3/8/8/8/3R3K w - d6 0 1", "e5d6"), 100);
}

TEST_F(SEETest, FourCaptureExchange) {
    EXPECT_EQ(compute_see("3r3k/3r4/8/3n4/5N2/1B6/8/7K w - - 0 1", "f4d5"), 200);
}

TEST_F(SEETest, TortureExchange) {
    EXPECT_EQ(compute_see("k3r3/1b2r2B/2bnrnB1/2nbrB2/qrrrRRRR/2nBRK2/2BNRN2/1B2Q3 b - - 0 1", "c3e4"), 200);
}
