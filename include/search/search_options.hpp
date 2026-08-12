#pragma once

#include "types.hpp"
#include "score.hpp"

#include <cstdint>
#include <optional>
#include <chrono>
#include <vector>

namespace bears_chess {

constexpr int MAX_DEPTH_LIMIT = MAX_PLY - 1;
constexpr size_t MAX_NODE_LIMIT = SIZE_MAX;
constexpr auto MAX_DEADLINE = std::chrono::steady_clock::time_point::max();

struct SearchOptions {
    std::optional<int> max_depth{};
    std::optional<size_t> max_node_count{};
    std::optional<std::chrono::steady_clock::time_point> deadline{};
    std::optional<std::vector<Move>> searchmoves{};
    int num_pvs{1};
};

} // namespace bears_chess