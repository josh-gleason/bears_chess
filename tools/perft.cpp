#include <iostream>
#include <chrono>
#include "board_utils.hpp"
#include "movegen.hpp"

using namespace std;
using namespace bears_chess;

uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;

    MoveList moves = generate_pseudo_legal_moves(board);
    uint64_t nodes = 0;
    for (const Move& move : moves) {
        UndoInfo undo = board.do_move(move);
        if (board.is_legal(move)) {
            nodes += perft(board, depth - 1);
        }
        board.undo_move(undo);
    }
    return nodes;
}

void perft_test(const std::string& fen, int max_depth) {
    Board board = load_fen(fen);
    std::cout << "======================== BEGIN PERFT TEST ================================" << std::endl;
    std::cout << board << std::endl;

    for (int depth = 1; depth <= max_depth; ++depth) {
        auto start = std::chrono::steady_clock::now();
        uint64_t nodes = perft(board, depth);
        auto end = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        std::cout << "Depth " << depth << ": " << nodes << " nodes"
                  << " (" << elapsed << " sec, " << (nodes/elapsed) << " nps)" << std::endl;
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
            std::cout << "Mismatch after do/undo for move: " << move << std::endl;
            std::cout << "Original:\n" << original << std::endl;
            std::cout << "After undo:\n" << board << std::endl;
            ++failures;
        } else if (depth > 1) {
            undo = board.do_move(move);
            test_do_undo(board, depth - 1);
            board.undo_move(undo);
        }
    }
    if (failures > 0) {
        std::cout << failures << " do/undo failures detected." << std::endl;
    }
}

void show_pseudolegal_moves(const std::string &fen) {
    Board board = load_fen(PERFT_POSITION_3_FEN);
    std::cout << set_board_format((board.side_to_move == Color::WHITE ? BoardFormat::NONE : BoardFormat::ORIENT_BLACK) | BoardFormat::HIDE_FEN);
    MoveList moves = generate_pseudo_legal_moves(board);
    std::cout << moves.size() << std::endl;
    for (Square from : BBSquareScan(board.occupied_by_color[idx(board.side_to_move)])) {
        Bitboard highlights = bb_square(from);
        for (int i = moves.size() - 1; i >= 0; --i) {
            if (moves[i].from == from) {
                std::cout << moves[i] << std::endl;
                highlights |= bb_square(moves[i].to);
                moves.erase(moves.begin() + i);
            }
        }
        std::cout << show_highlights(board, highlights);
    }

    if (moves.size() > 0) {
        std::cout << "ERROR: Should Be empty!!!!" << std::endl;
        for (auto move : moves) {
            std::cout << move << std::endl;
        }
    }
}

int main()
{
    // const std::vector<std::string> perft_fens = {
    //     INITIAL_POSITION_FEN,
    //     PERFT_POSITION_2_FEN,
    //     PERFT_POSITION_3_FEN,
    //     PERFT_POSITION_4_FEN,
    //     PERFT_POSITION_5_FEN,
    //     PERFT_POSITION_6_FEN
    // };

    // show_pseudolegal_moves(PERFT_POSITION_3_FEN);

    // for (auto fen : perft_fens) {
    //     test_do_undo(load_fen(fen), 2);
    // }

    perft_test(PERFT_POSITION_2_FEN, 5);

    return 0;
}
