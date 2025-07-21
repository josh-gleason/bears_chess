#pragma once

#include <print>
#include <format>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <optional>
#include <algorithm>
#include <numeric>
#include "board_utils.hpp"
#include "movegen.hpp"
#include "evaluation_utils.hpp"


template <>
struct std::hash<bears_chess::Move> {
    size_t operator()(const bears_chess::Move& move) const noexcept {
        size_t seed = 0;
        // Combine the hashes of the individual components of the Move
        seed ^= std::hash<int>()(static_cast<int>(move.from)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(static_cast<int>(move.to)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>()(static_cast<int>(move.move_type)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

namespace bears_chess {

using std::print, std::println;
using MoveGenType::LEGAL, MoveGenType::PSEUDO_LEGAL;

struct DepthStats {
    uint64_t nodes = 0;
    uint64_t captures = 0;
    uint64_t eps = 0;
    uint64_t castles = 0;
    uint64_t promotions = 0;
    uint64_t checks = 0;
    uint64_t discovered_checks = 0;
    uint64_t double_checks = 0;
    uint64_t checkmates = 0;

    std::string to_string() const {
        return std::format(FMT_STR, nodes, captures, eps, castles, promotions, checks, discovered_checks, double_checks, checkmates);
    }

    static std::string header_string() {
        return std::format(DepthStats::FMT_STR, "Nodes", "Captures", "En-Passants", "Castles", "Promotions", "Checks", "Discovery C.", "Double C.", "Checkmates");
    }

    inline void increment(const Board& board, const Move& last_move) {
        nodes++;
        captures += static_cast<int>(is_capture(last_move.move_type));
        eps += static_cast<int>(last_move.move_type == MoveType::EP_CAPTURE);
        castles += static_cast<int>(is_castle(last_move.move_type));
        promotions += static_cast<int>(is_promotion(last_move.move_type));
        checks += static_cast<int>(is_check(board));
        discovered_checks += static_cast<int>(is_discovered_check(board, last_move));
        double_checks += static_cast<int>(is_double_check(board));
        checkmates += static_cast<int>(is_checkmate(board));
    }

    const DepthStats operator+(const DepthStats& rhs) const {
        return {
            nodes + rhs.nodes,
            captures + rhs.captures,
            eps + rhs.eps,
            castles + rhs.castles,
            promotions + rhs.promotions,
            checks + rhs.checks,
            discovered_checks + rhs.discovered_checks,
            double_checks + rhs.double_checks,
            checkmates + rhs.checkmates
        };
    }

    private:
        static constexpr const char* FMT_STR = "{:>14}{:>14}{:>14}{:>14}{:>14}{:>14}{:>14}{:>14}{:>14}";
};

struct PerftResults {
    int depth = 0;
    std::string fen = "";
    uint64_t nodes = 0;
    double time = 0.0;
    std::optional<std::vector<DepthStats>> depth_stats;
    std::optional<std::unordered_map<Move, uint64_t>> move_nodes;
    std::optional<std::unordered_map<Move, DepthStats>> per_move_stats;

    const std::string to_string() const {
        std::string result = "Perft Results:\n";
        result += std::format("  FEN: {}\n", fen);
        if (depth_stats) {
            result += "  Depth Stats:\n";
            result += std::format("    {:>6}{}\n", "Depth", DepthStats::header_string());
            for (int depth = 0; depth < depth_stats->size(); ++depth) {
                result += std::format("    {:>6}{}\n", depth + 1, (*depth_stats)[depth].to_string());
            }
            result += '\n';
        }

        if (move_nodes) {
            result += "  Per-move:\n";
            result += std::format("    {:>6}", "Move");
            if (per_move_stats) {
                result += DepthStats::header_string();
            } else {
                result += std::format("{:>14}", "Nodes");
            }
            result += '\n';
            for (const auto& [move, count] : *move_nodes) {
                auto move_str = std::format("{:f}", move);
                result += std::format("    {:>6}", move_str);
                if (per_move_stats) {
                    result += per_move_stats->at(move).to_string();
                } else {
                    result += std::format("{:>14}", count);
                }
                result += "\n";
            }
            result += '\n';
        }

        double nps = time > 0 ? nodes / time : 0.0;
        result += std::format("  Depth {}: {} nodes ({:f} sec, {:g} nps)\n", depth, nodes, time, nps);

        return result;
    }
};

} // namespace bears_chess

template<>
struct std::formatter<bears_chess::PerftResults> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const bears_chess::PerftResults& results, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", results.to_string());
    }
};

namespace bears_chess {

template<MoveGenType move_gen_type>
bool is_legal(Board& board, const Move& last_move) {
    if constexpr (move_gen_type == LEGAL)
        return true;
    return board.is_legal(last_move);
}

template<MoveGenType move_gen_type=LEGAL, bool collect_stats=false>
inline uint64_t perft(Board& board, int depth, const std::vector<DepthStats>::iterator &stats) {
    if (depth == 0) {
        return 1;
    }

    uint64_t nodes = 0;
    auto moves = generate_moves<move_gen_type>(board);
    if constexpr (move_gen_type == LEGAL && !collect_stats) {
        if (depth == 1) {
            return moves.size();
        }
    }
    for (const Move& move : moves) {
        UndoInfo undo = board.do_move(move);
        if (is_legal<move_gen_type>(board, move)) {
            if constexpr (collect_stats) {
                stats->increment(board, move);
            }
            nodes += perft<move_gen_type, collect_stats>(board, depth - 1, stats + 1);
        }
        board.undo_move(undo);
    }

    return nodes;
}

template<MoveGenType move_gen_type=LEGAL, bool collect_stats=false, bool show_moves=false>
PerftResults run_perft(const Board& board_orig, int max_depth) {
    Board board = board_orig;

    std::vector<DepthStats> perft_stats(max_depth);
    std::vector<DepthStats> single_move_stats(max_depth - 1);
    std::unordered_map<Move, uint64_t> move_nodes;
    std::unordered_map<Move, DepthStats> per_move_stats;
    uint64_t nodes = 0;

    auto stats_iter = perft_stats.begin();
    auto single_move_stats_iter = single_move_stats.begin();

    auto start = std::chrono::steady_clock::now();
    MoveList moves = generate_moves<move_gen_type>(board);
    for (auto move : moves) {
        uint64_t m_nodes = 0;
        UndoInfo undo = board.do_move(move);
        if (is_legal<move_gen_type>(board, move)) {
            if constexpr (collect_stats) {
                stats_iter->increment(board, move);
                std::fill(single_move_stats.begin(), single_move_stats.end(), DepthStats());
            }
            m_nodes += perft<move_gen_type, collect_stats>(board, max_depth - 1, single_move_stats_iter);

            if constexpr (collect_stats) {
                for (int d = 1; d < max_depth; ++d) {
                    perft_stats[d] = perft_stats[d] + single_move_stats[d - 1];
                }
            }
            if constexpr (collect_stats && show_moves) {
                per_move_stats[move] = max_depth > 1 ? single_move_stats.back() : DepthStats();
            }
        }
        board.undo_move(undo);
        nodes += m_nodes;
        if constexpr (show_moves) {
            move_nodes[move] = m_nodes;
        }
    }
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    return PerftResults{
        max_depth,
        std::format("{:F}", board),
        nodes,
        elapsed,
        collect_stats ? std::optional<std::vector<DepthStats>>(std::move(perft_stats)) : std::nullopt,
        show_moves ? std::optional<std::unordered_map<Move, uint64_t>>(std::move(move_nodes)) : std::nullopt,
        (collect_stats && show_moves) ? std::optional<std::unordered_map<Move, DepthStats>>(std::move(per_move_stats)) : std::nullopt,
    };
}


template<MoveGenType move_gen_type=PSEUDO_LEGAL>
std::vector<Move> test_do_undo(const Board& original, int depth) {
    Board original_copy = Board(original);
    MoveList moves = generate_moves<move_gen_type>(original_copy);

    for (const Move& move : moves) {
        Board board = original; // Copy original
        UndoInfo undo = board.do_move(move);
        board.undo_move(undo);

        // Compare all relevant board state
        if (board != original) {
            Bitboard highlights = bb_square(move.from) | bb_square(move.to);

            bool differ = false;
            if (board.side_to_move != original.side_to_move) {
                println("side_to_move does not match");
                differ = true;
            }
            if (board.fullmove_number != original.fullmove_number) {
                println("fullmove_number does not match");
                differ = true;
            }
            if (board.castling_rights != original.castling_rights) {
                println("castling_rights does not match");
                differ = true;
            }
            if (board.ep_square != original.ep_square) {
                println("ep_square does not match");
                differ = true;
            }
            if (board.halfmove_clock != original.halfmove_clock) {
                println("halfmove_clock does not match");
                differ = true;
            }
            if (board.occupied != original.occupied) {
                println("occupied does not match");
                differ = true;
            }
            for (Piece p : iter<Piece>) {
                if (board.pieces[idx(Color::WHITE)][idx(p)] != board.pieces[idx(Color::WHITE)][idx(p)]) {
                    println("pieces[0][idx({})] does not match", p);
                    highlights |= (board.pieces[idx(Color::WHITE)][idx(p)] ^ board.pieces[idx(Color::WHITE)][idx(p)]);
                    differ = true;
                }
                if (board.pieces[idx(Color::BLACK)][idx(p)] != board.pieces[idx(Color::BLACK)][idx(p)]) {
                    println("pieces[1][idx({})] does not match", p);
                    highlights |= (board.pieces[idx(Color::BLACK)][idx(p)] ^ board.pieces[idx(Color::BLACK)][idx(p)]);
                    differ = true;
                }
            }
            if (board.occupied_by_color[idx(Color::WHITE)] != original.occupied_by_color[idx(Color::WHITE)]) {
                println("occupied_by_color[0] does not match");
                highlights |= (board.occupied_by_color[idx(Color::WHITE)] ^ original.occupied_by_color[idx(Color::WHITE)]);
                differ = true;
            }
            if (board.occupied_by_color[idx(Color::BLACK)] != original.occupied_by_color[idx(Color::BLACK)]) {
                println("occupied_by_color[1] does not match");
                highlights |= (board.occupied_by_color[idx(Color::BLACK)] ^ original.occupied_by_color[idx(Color::BLACK)]);
                differ = true;
            }
            if (board.king_sq[idx(Color::WHITE)] != original.king_sq[idx(Color::WHITE)]) {
                println("king_sq[0] does not match");
                highlights |= (bb_square(board.king_sq[idx(Color::WHITE)]) ^ bb_square(original.king_sq[idx(Color::WHITE)]));
                differ = true;
            }
            if (board.king_sq[idx(Color::BLACK)] != original.king_sq[idx(Color::BLACK)]) {
                println("king_sq[1] does not match");
                highlights |= (bb_square(board.king_sq[idx(Color::BLACK)]) ^ bb_square(original.king_sq[idx(Color::BLACK)]));
                differ = true;
            }
            for (Square sq : iter<Square>) {
                if (!board.is_occupied(sq) && !original.is_occupied(sq)) {
                    continue;
                }
                if (board.last_piece_sq[idx(sq)] != original.last_piece_sq[idx(sq)]) {
                    println("last_piece_sq[idx({})] does not match", sq);
                    highlights |= bb_square(sq);
                    differ = true;
                }
                if (board.last_color_sq[idx(sq)] != original.last_color_sq[idx(sq)]) {
                    println("last_color_sq[idx({})] does not match", sq);
                    highlights |= bb_square(sq);
                    differ = true;
                }
            }
            if (differ) {
                println("Mismatch after do/undo for move: {}", move);
                println("Original:\n{}", show_highlights(original, highlights));
                println("After undo:\n{}", show_highlights(board, highlights));
                return {move};
            }
        } else if (depth > 1) {
            undo = board.do_move(move);
            if (is_legal<move_gen_type>(board, move)) {
                // recurse only for legal moves, not designed to undo two illegal moves in a row
                auto movelist = test_do_undo<move_gen_type>(board, depth - 1);
                if (movelist.size() > 0) {
                    movelist.push_back(move);
                    return movelist;
                }
            }
            board.undo_move(undo);
        }
    }
    return {};
}

inline void show_move_list(const Board& board, const MoveList& moves_in) {
    println("# Moves: {}", moves_in.size());
    MoveList moves(moves_in.cbegin(), moves_in.cend());
    for (Square from : BBSquareScan(board.occupied_by_color[idx(board.side_to_move)])) {
        Bitboard highlights = bb_square(from);
        for (int i = moves.size() - 1; i >= 0; --i) {
            if (moves[i].from == from) {
                println("    {}", moves[i]);
                highlights |= bb_square(moves[i].to);
                moves.erase(moves.begin() + i);
            }
        }
        if (popcount(highlights) > 1) {
            println("{:+f}", show_highlights(board, highlights));
        }
    }

    if (moves.size() > 0) {
        println("ERROR: Should Be empty!!!!");
        for (auto move : moves) {
            println("{}", move);
        }
    }
}

template<MoveGenType move_gen_type=LEGAL>
inline void show_moves(const Board& board) {
    MoveList moves = generate_moves<move_gen_type>(board);
    show_move_list(board, moves);
}

} // namespace bears_chess