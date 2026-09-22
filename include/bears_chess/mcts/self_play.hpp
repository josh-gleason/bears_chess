#pragma once

#include "bears_chess/mcts/search.hpp"
#include "bears_chess/repetition.hpp"
#include "bears_chess/board_utils.hpp"

#include <stop_token>
#include <random>
#include <algorithm>
#include <vector>
#include <cmath>
#include <stdfloat>

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

struct SelfPlayRecord {
    std::string start_fen;
    std::vector<uint16_t> played_moves;
    std::vector<uint32_t> ply_offsets;

    std::vector<uint16_t> visited_moves;
    std::vector<float> visited_priors;
    std::vector<uint32_t> visited_visits;
    std::vector<float> visited_qs;

    float result_white{0.0f};
    GameTermination termination{GameTermination::PLY_LIMIT};
};

struct SelfPlayGame {
    std::vector<SelfPlayPly> plies;
    float result_white{0.0f};
    GameTermination termination{GameTermination::PLY_LIMIT};

    SelfPlayRecord record() const {
        SelfPlayRecord out;
        out.start_fen = plies.empty() ? std::string{} : get_fen(plies.front().board);
        out.result_white = result_white;
        out.termination = termination;
        
        out.ply_offsets.reserve(plies.size() + 1);
        out.played_moves.reserve(plies.size());
        out.visited_moves.reserve(plies.size() * 20);
        out.visited_priors.reserve(plies.size() * 20);
        out.visited_visits.reserve(plies.size() * 20);
        out.visited_qs.reserve(plies.size() * 20);

        out.ply_offsets.push_back(0);
        for (const SelfPlayPly& ply : plies) {
            out.played_moves.push_back(pack_move(ply.selected_move));
            for (const MCTSRootStats& s : ply.stats) {
                out.visited_moves.push_back(pack_move(s.move));
                out.visited_priors.push_back(s.prior);
                out.visited_visits.push_back(s.visits);
                out.visited_qs.push_back(s.q);
            }
            out.ply_offsets.push_back(static_cast<uint32_t>(out.visited_moves.size()));
        }
        return out;
    }

    static SelfPlayGame from_record(const SelfPlayRecord& record) {
        assert(record.played_moves.size() + 1 == record.ply_offsets.size());
        assert(record.visited_moves.size() == record.visited_priors.size());
        assert(record.visited_priors.size() == record.visited_visits.size());
        assert(record.visited_visits.size() == record.visited_qs.size());

        SelfPlayGame game{{}, record.result_white, record.termination};
        Board board = load_fen(record.start_fen);
        uint32_t k = 0;
        for (size_t i = 0; i < record.played_moves.size(); ++i) {
            SelfPlayPly ply{board, {}, unpack_move(record.played_moves[i])};
            assert(record.ply_offsets[i + 1] <= record.visited_moves.size());
            while (k < record.ply_offsets[i + 1]) {
                ply.stats.push_back(MCTSRootStats{
                    unpack_move(record.visited_moves[k]),
                    record.visited_priors[k],
                    record.visited_visits[k],
                    record.visited_qs[k]
                });
                ++k;
            }
            board.do_move(ply.selected_move);
            game.plies.push_back(std::move(ply));
        }
        return game;
    }

private:
    static constexpr uint16_t pack_move(const Move& move) {
        assert(!move.is_none());
        return static_cast<uint16_t>((idx(move.from) << 10) | (idx(move.to) << 4) | idx(move.move_type));
    }

    static constexpr Move unpack_move(uint16_t packed) {
        return Move{
            static_cast<Square>(packed >> 10),
            static_cast<Square>((packed >> 4) & 0x3F),
            static_cast<MoveType>(packed & 0xF)
        };
    }
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

