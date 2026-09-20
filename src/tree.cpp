#include "bears_chess/mcts/tree.hpp"
#include "bears_chess/mcts/reward.hpp"
#include "bears_chess/repetition.hpp"
#include "bears_chess/zobrist.hpp"

namespace bears_chess {

constexpr size_t MAX_RESERVE_NODES = 1048576;
constexpr size_t RESERVE_EDGES_PER_NODE = 40;
constexpr size_t MAX_RESERVE_EDGES = RESERVE_EDGES_PER_NODE * MAX_RESERVE_NODES;

Tree::Tree(float c_puct_, int max_ply_) :
    c_puct(c_puct_), max_ply(max_ply_)
{}

void Tree::reset() {
    nodes.clear();
    edges.clear();
}

void Tree::reserve(size_t simulations) {
    nodes.reserve(std::min(simulations + 1, MAX_RESERVE_NODES));
    edges.reserve(std::min(simulations * 40, MAX_RESERVE_EDGES));
}

bool Tree::empty() const {
    return nodes.empty();
}

Tree::Selection Tree::select(Board board, std::span<const ZobristHash> history) {
    StaticVector<EdgeIndex, MAX_PLY> path;
    StaticVector<ZobristHash, MAX_PLY + 1> hashes;
    std::optional<float> value{std::nullopt};
    bool child_exists = false;

    hashes.emplace_back(board.hash);

    if (nodes.empty()) {
        return {
            path,
            hashes,
            board,
            value,
            child_exists
        };
    }

    NodeIndex current_node_index = static_cast<NodeIndex>(0);
    while (true) {
        const Node& current_node = nodes[idx(current_node_index)];
        if (current_node.terminal) {
            value = current_node.reward;
            break;
        }

        EdgeIndex selected_edge_index = select_edge(current_node);
        Edge& selected_edge = edges[idx(selected_edge_index)];

        path.emplace_back(selected_edge_index);
        
        // virtual loss to support batching
        selected_edge.visits += 1;
        selected_edge.total_value -= VIRTUAL_LOSS;
        
        board.do_move(selected_edge.move);
        child_exists = (selected_edge.child_idx != NodeIndex::NONE);
        if (!child_exists) {
            if (static_cast<int>(path.size()) >= max_ply ||
                is_repetition(history, hashes.view(), board.hash, board.halfmove_clock) ||
                board.halfmove_clock >= 100
            ) {
                value = DRAW_REWARD;
            }
            hashes.emplace_back(board.hash);
            break;
        }
        hashes.emplace_back(board.hash);
        current_node_index = selected_edge.child_idx;
    }

    return {
        path,
        hashes,
        board,
        value,
        child_exists
    };
}

Tree::NodeIndex Tree::expand(std::optional<EdgeIndex> parent_index, const MoveList& moves, const PriorsList& priors) {
    assert(!moves.empty());
    assert(priors.size() == moves.size());
    assert(!parent_index || edges[idx(*parent_index)].child_idx == NodeIndex::NONE);

    EdgeIndex first_edge_index = static_cast<EdgeIndex>(edges.size());

    for (size_t i = 0; i < moves.size(); ++i) {
        const Move& move = moves[i];
        float prior = priors[i];
        edges.emplace_back(move, prior);
    }
    NodeIndex node_index = static_cast<NodeIndex>(nodes.size());
    
    uint32_t num_edges = moves.size();
    nodes.emplace_back(IndexRange<EdgeIndex>(first_edge_index, num_edges));

    if (parent_index) {
        Edge& parent = edges[idx(*parent_index)];
        parent.child_idx = node_index;
    }

    return node_index;
}

void Tree::mark_terminal(EdgeIndex parent_index, float reward) {
    assert(idx(parent_index) < edges.size());

    Edge& parent = edges[idx(parent_index)];

    assert(parent.child_idx == NodeIndex::NONE);
    
    parent.child_idx = static_cast<NodeIndex>(nodes.size());    
    const bool terminal = true;
    nodes.emplace_back(IndexRange<EdgeIndex>(), terminal, reward);
}

void Tree::add_root_noise(std::span<const float> noise, float epsilon) {
    assert(!nodes.empty());
    const Node& root = nodes[0];
    
    assert(noise.size() == root.edge_indices.size());

    for (size_t i = 0; i < noise.size(); ++i) {
        Edge& edge = edges[idx(root.edge_indices[i])];
        edge.prior = (1.0f - epsilon) * edge.prior + epsilon * noise[i];
    }
}

bool Tree::has_child(EdgeIndex edge_index) const {
    assert(idx(edge_index) < edges.size());
    return edges[idx(edge_index)].child_idx != NodeIndex::NONE;
}

void Tree::backup(std::span<const EdgeIndex> path, float reward) {
    for (auto edge_idx_it = path.rbegin(); edge_idx_it != path.rend(); ++edge_idx_it) {
        // reward is from perspective of the player moving,
        // e.g. being in checkmate is a score of -1, but playing a move that leads to checkmate state is 1
        reward *= -1;
        
        Edge& edge = edges[idx(*edge_idx_it)];

        // visit count already incremented in select with virtual loss
        edge.total_value += VIRTUAL_LOSS + reward;
    }
}

std::vector<MCTSRootStats> Tree::root_stats() const {
    assert(!nodes.empty());

    const Node& root = nodes[0];
    std::vector<MCTSRootStats> stats;
    stats.reserve(root.edge_indices.size());
    for (EdgeIndex edge_index : root.edge_indices) {
        const Edge& edge = edges[idx(edge_index)];
        stats.emplace_back(
            edge.move,
            edge.prior,
            edge.visits,
            static_cast<float>(edge.q())
        );
    }
    return stats;
}

float Tree::root_value() const {
    assert(!nodes.empty());

    double total_value = 0;
    size_t total_visits = 0;
    const Node& root = nodes[0];
    if (root.edge_indices.empty()) {
        return root.reward;
    }

    for (EdgeIndex edge_index : root.edge_indices) {
        const Edge& edge = edges[idx(edge_index)];
        total_value += edge.total_value;
        total_visits += edge.visits;
    }
    return static_cast<float>(total_value / std::max<size_t>(total_visits, 1));
}

} // namespace bears_chess