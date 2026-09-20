#pragma once

#include "bears_chess/mcts/evaluator.hpp"
#include "bears_chess/mcts/tree.hpp"
#include "bears_chess/mcts/reward.hpp"
#include "bears_chess/board.hpp"
#include "bears_chess/movegen.hpp"

#include <stop_token>

namespace bears_chess {

struct MCTSOptions {
    float c_puct{1.25f};
    int max_ply{MAX_PLY};
    int batch_size{1};
};

struct MCTSResult {
    std::vector<MCTSRootStats> moves;
    size_t simulations;
    float state_value{0};  // a.k.a. V

    std::optional<Move> best_move() const {
        if (moves.empty()) {
            return std::nullopt;
        }

        return std::max_element(
            moves.cbegin(),
            moves.cend(),
            [](const MCTSRootStats& a, const MCTSRootStats& b) {
                return a.visits < b.visits;
            }
        )->move;
    }
};

template <Evaluator EvaluatorClass>
class MCTS {
public:
    MCTS(EvaluatorClass evaluator_, MCTSOptions options_) :
        evaluator(std::move(evaluator_)),
        options(std::move(options_)),
        tree(options.c_puct, options.max_ply)
    {}

    MCTSResult go(
        std::stop_token stop_token,
        const Board& board,
        std::span<const ZobristHash> hash_history,
        size_t simulations
    ) {
        tree.reset();
        tree.reserve(simulations);

        {
            // initialize the root node
            auto [moves, in_check] = generate_moves_and_check<LegalPolicy>(board);
            if (moves.empty()) {
                bool checkmate = in_check;
                float reward = checkmate ? CHECKMATE_REWARD : DRAW_REWARD;
                return {{}, 0, reward};
            } else {
                EvalResult eval_result = evaluate_one(evaluator, board, moves);
                tree.expand(std::nullopt, moves, eval_result.priors);
            }
        }

        std::vector<Tree::Selection> pending;
        std::vector<EvalRequest> requests;
        pending.reserve(options.batch_size);
        requests.reserve(options.batch_size);

        size_t num_sims = 0;
        while (num_sims < simulations && !stop_token.stop_requested()) {
            pending.clear();
            requests.clear();

            for (int b = 0; b < options.batch_size && num_sims < simulations; ++b, ++num_sims) {
                Tree::Selection selection = tree.select(board, hash_history);

                if (selection.value) {
                    if (!selection.child_exists) {
                        tree.mark_terminal(selection.path.back(), *selection.value);
                    }
                    tree.backup(selection.path, *selection.value);
                    continue;
                }

                assert(!selection.child_exists);
                auto [moves, in_check] = generate_moves_and_check<LegalPolicy>(selection.board);
                if (moves.empty()) {
                    const float reward = in_check ? CHECKMATE_REWARD : DRAW_REWARD;
                    tree.mark_terminal(selection.path.back(), reward);
                    tree.backup(selection.path, reward);
                    continue;
                }

                requests.emplace_back(selection.board, std::move(moves));
                pending.emplace_back(std::move(selection));
            }

            if (requests.empty()) {
                continue;
            }

            const std::vector<EvalResult> results = evaluator.evaluate(requests);
            assert(results.size() == pending.size());

            for (size_t i = 0; i < pending.size(); ++i) {
                const Tree::EdgeIndex leaf_edge = pending[i].path.back();
                if (!tree.has_child(leaf_edge)) {
                    tree.expand(leaf_edge, requests[i].moves, results[i].priors);
                }
                tree.backup(pending[i].path, results[i].value);
            }
        }

        return {
            tree.root_stats(),
            num_sims,
            tree.root_value()
        };
    }
private:
    EvaluatorClass evaluator;
    MCTSOptions options;
    Tree tree;
};

}   // bears_chess