#include "bears_chess.hpp"
#include "bears_chess/mcts/encoding.hpp"

#include <gtest/gtest.h>

#include <set>

using namespace bears_chess;

static const std::vector<std::string> POSITIONS = {
    INITIAL_POSITION_FEN,
    PERFT_POSITION_2_FEN,
    PERFT_POSITION_3_FEN,
    PERFT_POSITION_4_FEN,
    PERFT_POSITION_5_FEN,
    PERFT_POSITION_6_FEN,
};

static const std::string KIWIPETE_WHITE = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
static const std::string KIWIPETE_MIRRORED_BLACK = "r3k2r/pppbbppp/2n2q1P/1P2p3/3pn3/BN2PNP1/P1PPQPB1/R3K2R b KQkq - 0 1";

static float plane_sum(const InputPlanes& planes, size_t plane) {
    float total = 0.0f;
    for (float value : planes.plane(plane)) {
        total += value;
    }
    return total;
}

class EncodingTest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

TEST_F(EncodingTest, PolicyTablesRoundTrip) {
    for (size_t i = 0; i < POLICY_SIZE; ++i) {
        EXPECT_EQ(POLICY_INDICES[POLICY_MOVES[i]], static_cast<int16_t>(i));
    }
}

TEST_F(EncodingTest, LegalMovesMapCorrectly) {
    for (const std::string& fen : POSITIONS) {
        Board board = load_fen(fen);
        std::set<size_t> seen;
        for (const Move& move : generate_moves<LegalPolicy>(board)) {
            const size_t index = policy_index(move, board.side_to_move);
            ASSERT_LT(index, POLICY_SIZE);
            // ensure index unique
            EXPECT_TRUE(seen.insert(index).second) << fen << " " << convert_move_to_uci(move);
            const PolicyMove& decoded = POLICY_MOVES[index];
            EXPECT_EQ(decoded.from, canonical_square(move.from, board.side_to_move));
            EXPECT_EQ(decoded.to, canonical_square(move.to, board.side_to_move));
            EXPECT_EQ(decoded.promotion, is_promotion(move.move_type) ? promote_to(move.move_type) : Piece::NONE);
        }
    }
}

TEST_F(EncodingTest, StartPositionPlanes) {
    InputPlanes planes = encode_board(load_fen(INITIAL_POSITION_FEN));
    const size_t own_pawns = InputPlanes::OWN_PIECES + idx(Piece::PAWN);
    const size_t opponent_pawns = InputPlanes::OPPONENT_PIECES + idx(Piece::PAWN);

    for (File file : iter<File>) {
        EXPECT_EQ(planes.at(own_pawns, square_of(file, Rank::_2)), 1.0f);
        EXPECT_EQ(planes.at(opponent_pawns, square_of(file, Rank::_7)), 1.0f);
    }
    EXPECT_EQ(plane_sum(planes, own_pawns), 8.0f);
    EXPECT_EQ(plane_sum(planes, opponent_pawns), 8.0f);
    EXPECT_EQ(planes.at(InputPlanes::OWN_PIECES + idx(Piece::KING), Square::E1), 1.0f);
    EXPECT_EQ(planes.at(InputPlanes::OPPONENT_PIECES + idx(Piece::KING), Square::E8), 1.0f);
    EXPECT_EQ(plane_sum(planes, InputPlanes::OWN_PIECES + idx(Piece::KING)), 1.0f);

    for (size_t plane : {InputPlanes::OWN_CASTLE_KING, InputPlanes::OWN_CASTLE_QUEEN,
                         InputPlanes::OPPONENT_CASTLE_KING, InputPlanes::OPPONENT_CASTLE_QUEEN}) {
        EXPECT_EQ(plane_sum(planes, plane), 64.0f);
    }
    EXPECT_EQ(plane_sum(planes, InputPlanes::EN_PASSANT), 0.0f);
    EXPECT_EQ(plane_sum(planes, InputPlanes::HALFMOVE_CLOCK), 0.0f);
}

TEST_F(EncodingTest, MirroredPositionsEncodeIdentically) {
    InputPlanes white = encode_board(load_fen(KIWIPETE_WHITE));
    InputPlanes black = encode_board(load_fen(KIWIPETE_MIRRORED_BLACK));
    for (size_t i = 0; i < InputPlanes::SIZE; ++i) {
        ASSERT_EQ(white.view()[i], black.view()[i]) << "index " << i;
    }
}

TEST_F(EncodingTest, EnPassantAndClockPlanes) {
    InputPlanes planes = encode_board(load_fen("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 30 3"));
    EXPECT_EQ(planes.at(InputPlanes::EN_PASSANT, Square::D6), 1.0f);
    EXPECT_EQ(plane_sum(planes, InputPlanes::EN_PASSANT), 1.0f);
    EXPECT_FLOAT_EQ(planes.at(InputPlanes::HALFMOVE_CLOCK, Square::A1), 0.3f);

    InputPlanes black = encode_board(load_fen("rnbqkbnr/pppp1ppp/8/8/3Pp3/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 3"));
    EXPECT_EQ(black.at(InputPlanes::EN_PASSANT, Square::D6), 1.0f);
}
