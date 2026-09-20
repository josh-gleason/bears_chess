#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/board.hpp"
#include "bears_chess/score.hpp"
#include "bears_chess/mcts/evaluator.hpp"

#include <optional>
#include <cmath>
#include <limits>
#include <algorithm>
#include <vector>
#include <span>

namespace bears_chess {

struct MCTSRootStats {
    Move move;
    float prior;
    uint32_t visits;
    float q;
};

class Tree {
public:
    enum class NodeIndex : uint32_t { NONE = UINT32_MAX };
    enum class EdgeIndex : uint32_t { NONE = UINT32_MAX };

    struct Selection {
        StaticVector<EdgeIndex, MAX_PLY> path;
        StaticVector<ZobristHash, MAX_PLY + 1> hashes;   // includes root hash in first slot
        Board board;
        std::optional<float> value;
        bool child_exists;
    };

    Tree(float c_puct, int max_ply);

    void reset();
    void reserve(size_t num_simulations);
    bool empty() const;

    Selection select(Board board, std::span<const ZobristHash> history);
    NodeIndex expand(std::optional<EdgeIndex> parent_index, const MoveList& moves, const PriorsList& priors);
    void mark_terminal(EdgeIndex parent_index, float reward);
    void backup(std::span<const EdgeIndex> path, float reward);
    bool has_child(EdgeIndex edge_index) const;

    std::vector<MCTSRootStats> root_stats() const;
    float root_value() const;

private:
    struct Edge {
        Edge(Move move_, float prior_) : prior(prior_), move(move_) {}

        double total_value{0};
        float prior;
        uint32_t visits{0};
        NodeIndex child_idx{NodeIndex::NONE};
        Move move;

        inline double q() const {
            return total_value / std::max<uint32_t>(visits, 1);
        }
    };

    struct Node {
        IndexRange<EdgeIndex> edge_indices;
        bool terminal{false};
        float reward{0};
    };

    inline float compute_puct_score(const uint32_t parent_visits, const Edge& edge) const {
        const double q = edge.q();
        const double u = c_puct * edge.prior * std::sqrt(static_cast<float>(parent_visits)) / (1 + edge.visits);
        return static_cast<float>(q + u);
    }

    inline EdgeIndex select_edge(const Node& node) const {
        assert(!node.edge_indices.empty());
        uint32_t total_visits = 0;
        for (auto edge_index : node.edge_indices) {
            total_visits += edges[idx(edge_index)].visits;
        }

        EdgeIndex best_edge_index = EdgeIndex::NONE;
        float best_score = -std::numeric_limits<float>::infinity();
        for (EdgeIndex edge_index : node.edge_indices) {
            float puct_score = compute_puct_score(total_visits, edges[idx(edge_index)]);
            if (puct_score > best_score) {
                best_edge_index = edge_index;
                best_score = puct_score;
            }
        }
        return best_edge_index;
    }

    float c_puct;
    int max_ply;
    std::vector<Node> nodes{};
    std::vector<Edge> edges{};
};

} // namespace bears_chess