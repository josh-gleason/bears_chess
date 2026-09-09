#pragma once
#include "board.hpp"
#include "types.hpp"
#include "evaluation.hpp"
#include "movegen/movelist.hpp"

#include <vector>
#include <span>
#include <cassert>
#include <cmath>

namespace bears_chess {

struct EvalRequest {
    Board board;
    MoveList moves;     // never empty
};

struct EvalResult {
    StaticVector<float, MoveList::max_length> priors;   // always parallel to requests moves
    float value;                                        // in [-1, 1] from perspective of board.side_to_move
};


template <typename T>
concept Evaluator = requires(T& e, std::span<const EvalRequest> requests) {
    { e.evaluate(requests) } -> std::same_as<std::vector<EvalResult>>;
};


class UniformEvaluator {
public:
    // simple method using uniform priors and piece value
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