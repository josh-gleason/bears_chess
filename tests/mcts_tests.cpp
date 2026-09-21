#include "bears_chess.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

using namespace bears_chess;

namespace {

struct WhiteDisadvantageEvaluator {
    float magnitude;

    std::vector<EvalResult> evaluate(std::span<const EvalRequest> requests) {
        std::vector<EvalResult> results;
        for (const EvalRequest& request : requests) {
            EvalResult& result = results.emplace_back();
            for (size_t i = 0; i < request.moves.size(); ++i) {
                result.priors.emplace_back(1.0f / request.moves.size());
            }
            result.value = request.board.side_to_move == Color::WHITE ? -magnitude : magnitude;
        }
        return results;
    }
};

struct FavoredMoveEvaluator {
    Move favored;

    std::vector<EvalResult> evaluate(std::span<const EvalRequest> requests) {
        std::vector<EvalResult> results;
        for (const EvalRequest& request : requests) {
            EvalResult& result = results.emplace_back();
            const size_t n = request.moves.size();
            const bool has_favored = std::find(request.moves.begin(), request.moves.end(), favored) != request.moves.end();
            for (const Move& move : request.moves) {
                if (!has_favored) {
                    result.priors.emplace_back(1.0f / n);
                } else if (move == favored) {
                    result.priors.emplace_back(0.9f);
                } else {
                    result.priors.emplace_back(0.1f / (n - 1));
                }
            }
            result.value = 0.0f;
        }
        return results;
    }
};

struct BatchRecordingEvaluator {
    std::vector<size_t>* batch_sizes;

    std::vector<EvalResult> evaluate(std::span<const EvalRequest> requests) {
        batch_sizes->push_back(requests.size());
        return UniformEvaluator{}.evaluate(requests);
    }
};

const std::string MATE_IN_1 = "6k1/5ppp/8/8/8/8/8/4R2K w - - 0 1";
const std::string MATED = "4R1k1/5ppp/8/8/8/8/8/7K b - - 0 1";
const std::string STALEMATE = "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1";
const std::string MATE_IN_2 = "kbK5/pp6/1P6/8/8/8/8/R7 w - - 0 1";
const std::string QUEEN_UP = "4k3/8/8/8/8/8/8/3QK3 w - - 10 20";

template <Evaluator E>
MCTSResult search(
    E evaluator,
    const std::string& fen,
    size_t simulations,
    MCTSOptions options = {},
    std::span<const ZobristHash> history = {})
{
    MCTS<E> mcts(std::move(evaluator), options);
    return mcts.go(std::stop_token{}, load_fen(fen), history, simulations);
}

const MCTSRootStats& most_visited(const MCTSResult& result) {
    return *std::max_element(
        result.moves.begin(), result.moves.end(),
        [](const MCTSRootStats& a, const MCTSRootStats& b) {
            return a.visits < b.visits;
        }
    );
}

const MCTSRootStats& stats_for_move(const MCTSResult& result, const std::string& uci) {
    return *std::find_if(
        result.moves.begin(),
        result.moves.end(),
        [&](const MCTSRootStats& s) {
            return convert_move_to_uci(s.move) == uci;
        }
    );
}

uint32_t total_visits(const MCTSResult& result) {
    return std::accumulate(
        result.moves.begin(),
        result.moves.end(),
        uint32_t{0u},
        [](uint32_t total, const MCTSRootStats& s) {
            return total + s.visits;
        }
    );
}

} // namespace

class MCTSTest : public ::testing::Test {
protected:
    void SetUp() override {
        init();
    }
};

TEST_F(MCTSTest, BackupSignCorrectUpToRoot) {
    MCTSResult result = search(WhiteDisadvantageEvaluator{0.5f}, INITIAL_POSITION_FEN, 200);
    for (const MCTSRootStats& s : result.moves) {
        EXPECT_FLOAT_EQ(s.q, -0.5f);
    }
    EXPECT_FLOAT_EQ(result.state_value, -0.5f);
}

TEST_F(MCTSTest, FavoredPriorsSelectMove) {
    Board board = load_fen(INITIAL_POSITION_FEN);
    Move favored = convert_uci_to_move("a2a3", board);
    MCTSResult result = search(FavoredMoveEvaluator{favored}, INITIAL_POSITION_FEN, 500);
    EXPECT_EQ(most_visited(result).move, favored);
    EXPECT_GT(stats_for_move(result, "a2a3").visits, 250u);
}

TEST_F(MCTSTest, MateInOneFindsMate) {
    MCTSResult result = search(UniformEvaluator{}, MATE_IN_1, 800);
    const MCTSRootStats& best = most_visited(result);
    EXPECT_EQ(convert_move_to_uci(best.move), "e1e8");
    EXPECT_GT(best.visits, 400u);
    EXPECT_FLOAT_EQ(best.q, 1.0f);
}

TEST_F(MCTSTest, EasyMateInTwoFindsMove) {
    MCTSResult result = search(UniformEvaluator{}, MATE_IN_2, 3000);
    const MCTSRootStats& best = most_visited(result);
    EXPECT_EQ(convert_move_to_uci(best.move), "a1a6");
    EXPECT_GT(best.q, 0.5f);
}

TEST_F(MCTSTest, TerminalRoot) {
    MCTSResult mated = search(UniformEvaluator{}, MATED, 100);
    EXPECT_TRUE(mated.moves.empty());
    EXPECT_EQ(mated.simulations, 0u);
    EXPECT_FLOAT_EQ(mated.state_value, CHECKMATE_REWARD);

    MCTSResult stalemated = search(UniformEvaluator{}, STALEMATE, 100);
    EXPECT_TRUE(stalemated.moves.empty());
    EXPECT_EQ(stalemated.simulations, 0u);
    EXPECT_FLOAT_EQ(stalemated.state_value, DRAW_REWARD);
}

TEST_F(MCTSTest, MaxPlyBoundaryResultsInDrawReward) {
    MCTSResult capped = search(UniformEvaluator{}, QUEEN_UP, 300, MCTSOptions{.max_ply = 1});
    for (const MCTSRootStats& s : capped.moves) {
        EXPECT_FLOAT_EQ(s.q, DRAW_REWARD);
    }
    EXPECT_FLOAT_EQ(capped.state_value, DRAW_REWARD);

    MCTSResult uncapped = search(UniformEvaluator{}, QUEEN_UP, 300);
    EXPECT_GT(uncapped.state_value, 0.5f);
}

TEST_F(MCTSTest, RepetitionCheck) {
    Board board = load_fen(QUEEN_UP);
    Move repeating = convert_uci_to_move("e1e2", board);
    Board after = board;
    after.do_move(repeating);
    std::vector<ZobristHash> history{board.hash, after.hash, board.hash, after.hash};

    MCTSResult result = search(UniformEvaluator{}, QUEEN_UP, 300, MCTSOptions{}, history);
    EXPECT_GT(stats_for_move(result, "e1e2").visits, 0u);
    EXPECT_FLOAT_EQ(stats_for_move(result, "e1e2").q, DRAW_REWARD);
    for (const MCTSRootStats& s : result.moves) {
        if (s.move != repeating && s.visits > 0) {
            EXPECT_GT(s.q, 0.5f);
        }
    }
}

TEST_F(MCTSTest, MateInOneWithBatching) {
    for (int batch_size : {1, 8, 64}) {
        MCTSResult result = search(UniformEvaluator{}, MATE_IN_1, 800, MCTSOptions{.batch_size = batch_size});
        EXPECT_EQ(total_visits(result), 800u) << "batch_size " << batch_size;
        EXPECT_EQ(result.simulations, 800u) << "batch_size " << batch_size;
        const MCTSRootStats& best = most_visited(result);
        EXPECT_EQ(convert_move_to_uci(best.move), "e1e8") << "batch_size " << batch_size;
        EXPECT_FLOAT_EQ(best.q, 1.0f) << "batch_size " << batch_size;
    }
}

TEST_F(MCTSTest, MaxBatchSizeAchieved) {
    std::vector<size_t> batch_sizes;
    search(BatchRecordingEvaluator{&batch_sizes}, INITIAL_POSITION_FEN, 2000, MCTSOptions{.batch_size = 16});
    ASSERT_FALSE(batch_sizes.empty());
    EXPECT_EQ(*std::max_element(batch_sizes.begin(), batch_sizes.end()), 16u);
    EXPECT_GT(std::count(batch_sizes.begin(), batch_sizes.end(), 16u), batch_sizes.size() / 2);
}

TEST_F(MCTSTest, StopEarlyFromThread) {
    MCTS<UniformEvaluator> mcts(UniformEvaluator{}, MCTSOptions{});
    Board board = load_fen(INITIAL_POSITION_FEN);
    std::vector<ZobristHash> history;
    std::stop_source source;
    std::thread stopper([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        source.request_stop();
    });
    const auto started = std::chrono::steady_clock::now();
    MCTSResult result = mcts.go(source.get_token(), board, history, 1'000'000);
    const auto elapsed = std::chrono::steady_clock::now() - started;
    stopper.join();
    EXPECT_LT(result.simulations, 1'000'000u);
    EXPECT_LT(elapsed, std::chrono::seconds(5));
    EXPECT_EQ(total_visits(result), result.simulations);
}

TEST_F(MCTSTest, RootNoiseHasImpact) {
    MCTSOptions noisy{.dirichlet_epsilon = 0.25f, .seed = 7};
    MCTSResult first = search(UniformEvaluator{}, INITIAL_POSITION_FEN, 50, noisy);
    MCTSResult second = search(UniformEvaluator{}, INITIAL_POSITION_FEN, 50, noisy);
    MCTSResult other = search(
        UniformEvaluator{},
        INITIAL_POSITION_FEN,
        50,
        MCTSOptions{.dirichlet_epsilon = 0.25f, .seed = 8}
    );
    
    float total = 0.0f;
    bool any_differs = false;
    for (size_t i = 0; i < first.moves.size(); ++i) {
        total += first.moves[i].prior;
        any_differs |= first.moves[i].prior != 1.0f / first.moves.size();
        EXPECT_FLOAT_EQ(first.moves[i].prior, second.moves[i].prior);
    }   
    EXPECT_NEAR(total, 1.0f, 1e-5f);
    EXPECT_TRUE(any_differs);
    EXPECT_NE(first.moves[0].prior, other.moves[0].prior);
}

TEST_F(MCTSTest, SelfPlayConsistency) {
    MCTS<UniformEvaluator> mcts(UniformEvaluator{}, MCTSOptions{.seed = 3});
    SelfPlay driver(SelfPlayOptions{.simulations = 50, .max_plies = 40, .seed = 5});
    SelfPlayGame game = driver.play(mcts, load_fen(INITIAL_POSITION_FEN));

    ASSERT_FALSE(game.plies.empty());
    for (size_t i = 0; i < game.plies.size(); ++i) {
        const SelfPlayPly& ply = game.plies[i];
        EXPECT_TRUE(std::any_of(ply.stats.begin(), ply.stats.end(),
            [&](const MCTSRootStats& s) {
                return s.move == ply.selected_move;
            }
        ));
        if (i + 1 < game.plies.size()) {
            Board next = ply.board;
            next.do_move(ply.selected_move);
            EXPECT_EQ(next, game.plies[i + 1].board);
        }
    }
    EXPECT_EQ(game.result_white != 0.0f, game.termination == GameTermination::CHECKMATE);
}
