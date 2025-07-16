#include <format>
#include <print>
#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include "board_utils.hpp"
#include "movegen.hpp"

using namespace std;
using namespace bears_chess;

struct DepthStats {
    uint64_t nodes{};
    uint64_t captures{};
    uint64_t eps{};
    uint64_t castles{};
    uint64_t promotions{};
    uint64_t checks{};
    uint64_t discovery_checks{};
    uint64_t double_checks{};
    uint64_t checkmates{};

    std::string to_string(int depth) const {
        return std::format(FMT_STR, depth, nodes, captures, eps, castles, promotions, checks, discovery_checks, double_checks, checkmates);
    }

    static std::string header_string() {
        return std::format(DepthStats::FMT_STR, "depth", "nodes", "capture", "e.p.", "castle", "promotion", "check", "disc-check", "dbl-check", "checkmate");
    }

    inline void increment(const Board& board, const Move& last_move) {
        nodes++;
        captures += static_cast<int>(is_capture(last_move.move_type));
        eps += static_cast<int>(last_move.move_type == MoveType::EP_CAPTURE);
        castles += static_cast<int>(is_castle(last_move.move_type));
        promotions += static_cast<int>(is_promotion(last_move.move_type));
    }

    private:
        static constexpr const char* FMT_STR = "{:<7}{:<14}{:<14}{:<14}{:<14}{:<14}{:<14}{:<14}{:<14}{:<14}";
};

template<MoveGenType T>
inline bool is_legal(const Board& board, const Move& last_move) {
    if constexpr (T == MoveGenType::LEGAL) {
        return true;
    } else {
        return board.is_legal(last_move);
    }
}

template<MoveGenType T, bool CollectStats = false>
inline uint64_t perft(Board& board, int depth, const std::vector<DepthStats>::iterator &stats) {
    if (depth == 0) {
        return 1;
    }

    uint64_t nodes = 0;
    auto moves = generate_moves<T>(board);
    if constexpr (T == MoveGenType::LEGAL && !CollectStats) {
        if (depth == 1) {
            return moves.size();
        }
    }
    for (const Move& move : moves) {
        UndoInfo undo = board.do_move(move);
        if (is_legal<T>(board, move)) {
            if constexpr (CollectStats) {
                stats->increment(board, move);
            }
            nodes += perft<T, CollectStats>(board, depth - 1, stats + 1);
        }
        board.undo_move(undo);
    }

    return nodes;
}

template<MoveGenType T, bool CollectStats = false>
void run_perft(const std::string& fen, int max_depth) {
    Board board = load_fen(fen);
    println("======================== BEGIN PERFT TEST ================================");
    println("{}", board);

    std::vector<DepthStats> perft_stats(max_depth);
    auto stats_iter = perft_stats.begin();
    uint64_t nodes = 0;

    auto start = std::chrono::steady_clock::now();
    MoveList moves = generate_moves<T>(board);
    for (auto move : moves) {
        uint64_t move_nodes = 0;
        UndoInfo undo = board.do_move(move);
        if (is_legal<T>(board, move)) {
            if constexpr (CollectStats) {
                stats_iter->increment(board, move);
            }
            move_nodes += perft<T, CollectStats>(board, max_depth - 1, stats_iter + 1);
        }
        board.undo_move(undo);
        nodes += move_nodes;
        println("{:s}: {}", move, move_nodes);
    }
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    println("Depth {}: {} nodes ({} sec, {:g} nps)", max_depth, nodes, elapsed, nodes / elapsed);

    if constexpr (CollectStats) {
        println("{}", DepthStats::header_string());
        for (int depth = 0; depth < max_depth; ++depth) {
            println("{}", perft_stats[depth].to_string(depth + 1));
        }
    }
}

void test_do_undo(const Board& original, int depth) {
    MoveList moves = generate_pseudo_legal_moves(original);

    int failures = 0;
    for (const Move& move : moves) {
        Board board = original; // Copy original
        UndoInfo undo = board.do_move(move);
        board.undo_move(undo);

        // Compare all relevant board state
        if (board != original) {
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
                    differ = true;
                }
                if (board.pieces[idx(Color::BLACK)][idx(p)] != board.pieces[idx(Color::BLACK)][idx(p)]) {
                    println("pieces[1][idx({})] does not match", p);
                    differ = true;
                }
            }
            if (board.occupied_by_color[idx(Color::WHITE)] != original.occupied_by_color[idx(Color::WHITE)]) {
                println("occupied_by_color[0] does not match");
                differ = true;
            }
            if (board.occupied_by_color[idx(Color::BLACK)] != original.occupied_by_color[idx(Color::BLACK)]) {
                println("occupied_by_color[1] does not match");
                differ = true;
            }
            if (board.king_sq[idx(Color::WHITE)] != original.king_sq[idx(Color::WHITE)]) {
                println("king_sq[0] does not match");
                differ = true;
            }
            if (board.king_sq[idx(Color::BLACK)] != original.king_sq[idx(Color::BLACK)]) {
                println("king_sq[1] does not match");
                differ = true;
            }
            for (Square sq : iter<Square>) {
                if (!board.is_occupied(sq) && !original.is_occupied(sq)) {
                    continue;
                }
                if (board.last_piece_sq[idx(sq)] != original.last_piece_sq[idx(sq)]) {
                    println("last_piece_sq[idx({})] does not match", sq);
                    differ = true;
                }
                if (board.last_color_sq[idx(sq)] != original.last_color_sq[idx(sq)]) {
                    println("last_color_sq[idx({})] does not match", sq);
                    differ = true;
                }
            }
            if (differ) {
                println("Mismatch after do/undo for move: {}", move);
                println("Original:\n{}", original);
                println("After undo:\n{}", board);
                ++failures;
            }
        } else if (depth > 1) {
            undo = board.do_move(move);
            test_do_undo(board, depth - 1);
            board.undo_move(undo);
        }
    }
    if (failures > 0) {
        println("{} do/undo failures detected", failures);
    }
}

void show_move_list(const Board& board, const MoveList& moves_in) {
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
        println("{:+f}", show_highlights(board, highlights));
    }

    if (moves.size() > 0) {
        println("ERROR: Should Be empty!!!!");
        for (auto move : moves) {
            println("{}", move);
        }
    }
}

void show_pseudolegal_moves(const std::string &fen) {
    Board board = load_fen(fen);
    MoveList moves = generate_pseudo_legal_moves(board);
    show_move_list(board, moves);
}

void show_legal_moves(const std::string &fen) {
    Board board = load_fen(fen);
    MoveList moves = generate_legal_moves(board);
    show_move_list(board, moves);
}

int main()
{
    // show_pseudolegal_moves(PERFT_POSITION_3_FEN);
    // show_legal_moves(PERFT_POSITION_3_FEN);

    // test_do_undo(load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/P1N2Q2/1PPBBPpP/2KR3R b kq - 0 2"), 2);
    // test_do_undo(load_fen(PERFT_POSITION_2_FEN), 4);

    // perft_tree(PERFT_POSITION_2_FEN, 5);

    // perft_speed(INITIAL_POSITION_FEN, 6);
    // perft_speed(PERFT_POSITION_2_FEN, 5);
    // perft_speed(PERFT_POSITION_3_FEN, 6);
    // perft_speed(PERFT_POSITION_4_FEN, 6);
    // perft_speed(PERFT_POSITION_5_FEN, 5);
    // perft_speed(PERFT_POSITION_6_FEN, 5);

    run_perft<MoveGenType::PSEUDO_LEGAL, false>(INITIAL_POSITION_FEN, 1);
    run_perft<MoveGenType::PSEUDO_LEGAL, false>(INITIAL_POSITION_FEN, 6);
    // run_perft<MoveGenType::PSEUDO_LEGAL, true>(PERFT_POSITION_2_FEN, 4);

    return 0;
}
