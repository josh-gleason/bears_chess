#include "bears_chess.hpp"

#include <gtest/gtest.h>

using namespace bears_chess;

class ZobristTest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

template<MoveGenPolicy policy=LegalPolicy>
bool test_zobrist_hash_(Board& board, int depth) {
    if (depth == 0) {
        return true;
    }

    MoveList moves = generate_moves<policy>(board);
    for (const Move& move : moves) {
        ZobristHash original_hash = board.hash;
        UndoInfo undo = board.do_move(move);
        ZobristHash computed_hash = compute_zobrist_hash(board);

        if (board.hash != computed_hash) {
            board.undo_move(undo);
            return false;
        }
        
        if (board.is_legal(move)) {
            if (!test_zobrist_hash_<policy>(board, depth - 1)) {
                board.undo_move(undo);
                return false;
            }
        }

        board.undo_move(undo);

        if (board.hash != original_hash) {
            return false;
        }
    }

    return true;
}

template<MoveGenPolicy policy=LegalPolicy>
bool test_zobrist_hash(const Board& board, int depth) {
    Board board_copy = board;

    ZobristHash computed_hash = compute_zobrist_hash(board_copy);
    if (board_copy.hash != computed_hash) {
        return false;
    }

    return test_zobrist_hash_<policy>(board_copy, depth);
}

TEST_F(ZobristTest, Legal_StartingPosition) {
    EXPECT_TRUE(test_zobrist_hash(Board(), 4));
}

TEST_F(ZobristTest, Pseudolegal_StartingPosition) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(Board(), 4));
}

TEST_F(ZobristTest, Legal_KiwipeteDepth4) {
    EXPECT_TRUE(test_zobrist_hash(load_fen(PERFT_POSITION_2_FEN), 4));
}

TEST_F(ZobristTest, Pseudolegal_KiwipeteDepth4) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(load_fen(PERFT_POSITION_2_FEN), 4));
}

TEST_F(ZobristTest, Legal_Pos3Depth4) {
    EXPECT_TRUE(test_zobrist_hash(load_fen(PERFT_POSITION_3_FEN), 4));
}

TEST_F(ZobristTest, Pseudolegal_Pos3Depth4) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(load_fen(PERFT_POSITION_3_FEN), 4));
}

TEST_F(ZobristTest, Legal_Pos4Depth4) {
    EXPECT_TRUE(test_zobrist_hash(load_fen(PERFT_POSITION_4_FEN), 4));
}

TEST_F(ZobristTest, Pseudolegal_Pos4Depth4) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(load_fen(PERFT_POSITION_4_FEN), 4));
}

TEST_F(ZobristTest, Legal_Pos5Depth4) {
    EXPECT_TRUE(test_zobrist_hash(load_fen(PERFT_POSITION_5_FEN), 4));
}

TEST_F(ZobristTest, Pseudolegal_Pos5Depth4) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(load_fen(PERFT_POSITION_5_FEN), 4));
}

TEST_F(ZobristTest, Legal_Pos6Depth4) {
    EXPECT_TRUE(test_zobrist_hash(load_fen(PERFT_POSITION_6_FEN), 4));
}

TEST_F(ZobristTest, Pseudolegal_Pos6Depth4) {
    EXPECT_TRUE(test_zobrist_hash<PseudoLegalPolicy>(load_fen(PERFT_POSITION_6_FEN), 4));
}

TEST_F(ZobristTest, InitializationProducesNonZeroHash) {
    Board board;
    EXPECT_NE(board.hash, ZobristHash::ZERO);
}
