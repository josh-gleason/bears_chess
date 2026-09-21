#pragma once
#include "bears_chess/board.hpp"
#include "bears_chess/types.hpp"
#include "bears_chess/evaluation.hpp"
#include "bears_chess/movegen/movelist.hpp"

#include <vector>
#include <span>
#include <cassert>
#include <cmath>

namespace bears_chess {

struct EvalRequest {
    Board board;
    MoveList moves;     // never empty
};

typedef StaticVector<float, MoveList::max_length> PriorsList;

struct EvalResult {
    PriorsList priors;      // always parallel to requests moves
    float value;            // in [-1, 1] from perspective of board.side_to_move
};


template <typename T>
concept Evaluator = requires(T& e, std::span<const EvalRequest> requests) {
    { e.evaluate(requests) } -> std::same_as<std::vector<EvalResult>>;
};

template <Evaluator E>
EvalResult evaluate_one(E& evaluator, const Board& board, const MoveList& moves) {
    const EvalRequest request{board, moves};
    return evaluator.evaluate(std::span(&request, 1)).front();
}

class UniformEvaluator {
public:
    // simple evaluator giving uniform priors and piece value
    std::vector<EvalResult> evaluate(std::span<const EvalRequest> requests) {
        std::vector<EvalResult> results;
        results.reserve(requests.size());
        for (const EvalRequest& request : requests) {
            assert(!request.moves.empty());
            EvalResult &result = results.emplace_back();
            const size_t n = request.moves.size();
            for (size_t i = 0; i < n; ++i) {
                result.priors.emplace_back(1.0f / n);
            }
            result.value = std::tanh(bears_chess::evaluate(request.board) / 400.0f);

            assert(-1.0f <= result.value && result.value <= 1.0f);
            assert(result.priors.size() == request.moves.size());
        }
        assert(results.size() == requests.size());
        return results;
    }
};

} // namespace bears_chess