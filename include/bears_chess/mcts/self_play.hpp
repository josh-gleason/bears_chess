#pragma once

#include "bears_chess/mcts/search.hpp"
#include "bears_chess/repetition.hpp"

#include <stop_token>
#include <random>
#include <algorithm>
#include <vector>
#include <cmath>

namespace bears_chess {

enum class GameTermination : uint8_t {
    CHECKMATE,
    STALEMATE,
    REPETITION,
    FIFTY_MOVE,
    PLY_LIMIT,
    STOPPED
};

struct SelfPlayOptions {
    size_t simulations{800};
    int temperature_plies{30};
    float temperature{1.0f};
    int max_plies{512};
    uint64_t seed{0};
};

struct SelfPlayPly {
    Board board;
    std::vector<MCTSRootStats> stats;
    Move selected_move;
};

struct SelfPlayGame {
    std::vector<SelfPlayPly> plies;
    float result_white{0.0f};
    GameTermination termination{GameTermination::PLY_LIMIT};
};

class SelfPlay {
public:
    SelfPlay(SelfPlayOptions options_ = {}) : options(options_), rng(options.seed) {}

    template<Evaluator E>
    SelfPlayGame play(MCTS<E>& mcts, Board board, std::stop_token stop_token = {}) {
        SelfPlayGame game;
        std::vector<ZobristHash> hash_history;

        while (true) {
            if (static_cast<int>(game.plies.size()) >= options.max_plies) {
                game.termination = GameTermination::PLY_LIMIT;
                break;
            }
            if (board.halfmove_clock >= 100) {
                game.termination = GameTermination::FIFTY_MOVE;
                break;
            }
            if (is_repetition<false>(hash_history, {}, board.hash, board.halfmove_clock)) {
                game.termination = GameTermination::REPETITION;
                break;
            }

            MCTSResult result = mcts.go(stop_token, board, hash_history, options.simulations);

            if (stop_token.stop_requested()) {
                game.termination = GameTermination::STOPPED;
                break;
            }
            if (result.moves.empty()) {
                const bool mated = (result.state_value == CHECKMATE_REWARD);
                game.termination = mated ? GameTermination::CHECKMATE : GameTermination::STALEMATE;
                if (mated) {
                    game.result_white = board.side_to_move == Color::WHITE ? -1.0f : 1.0f;
                }
                break;
            }

            const Move selected_move = choose_move(result.moves, temperature_at(game.plies.size()));
            game.plies.emplace_back(board, std::move(result.moves), selected_move);
            hash_history.push_back(board.hash);
            board.do_move(selected_move);
        }

        return game;
    }

private:
    float temperature_at(size_t ply) {
        // TODO: investigate if smooth schedule for temperature would be better
        if (ply >= options.temperature_plies) {
            return 0.0f;
        }
        return options.temperature;
    }

    Move choose_move(const std::vector<MCTSRootStats>& stats, float temperature) {
        if (temperature <= 0.0f) {
            return std::max_element(stats.cbegin(), stats.cend(),
                [](const MCTSRootStats& a, const MCTSRootStats& b) {
                    return a.visits < b.visits;
                })->move;
        }
        StaticVector<double, MAX_PLY> weights;
        weights.resize(stats.size());
        for (size_t i = 0; i < stats.size(); ++i) {
            weights[i] = std::pow(static_cast<double>(stats[i].visits), 1.0 / temperature);
        }
        std::discrete_distribution<size_t> choice_distribution(weights.begin(), weights.end());
        return stats[choice_distribution(rng)].move;
    }

    SelfPlayOptions options;
    std::mt19937_64 rng;
};

} // bears_chess

