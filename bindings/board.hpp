#pragma once
#include <bears_chess/types.hpp>
#include <bears_chess/movegen.hpp>
#include <bears_chess/board_utils.hpp>
#include <bears_chess/position_info.hpp>

#include <vector>

using namespace bears_chess;

namespace bears_chess_py {

class PyBoard {
public:
    PyBoard(const std::string& fen = INITIAL_POSITION_FEN) : board(load_fen(fen)) {
        hashes.push_back(board.hash);
    }

    std::string fen() const {
        return get_fen(board);
    }

    std::vector<Move> legal_moves() const {
        const MoveList moves = generate_moves<LegalPolicy>(board);
        return std::vector<Move>(moves.cbegin(), moves.cend());
    }

    void do_move(const Move& move) {
        if (!is_legal_move(move)) {
            throw std::invalid_argument("illegal move");
        }
        do_move_unchecked(move);
    }

    void do_move_unchecked(const Move& move) {
        undo.push_back(board.do_move(move));
        hashes.push_back(board.hash);
    }

    void undo_move() {
        if (undo.empty()) {
            throw std::runtime_error("undo_move() called at root position");
        }
        board.undo_move(undo.back());
        undo.pop_back();
        hashes.pop_back();
    }

    bool is_check() const {
        return bears_chess::is_check(board);
    }

    bool is_checkmate() const {
        return bears_chess::is_checkmate(board);
    }

    bool is_stalemate() const {
        return bears_chess::is_stalemate(board);
    }

    bool is_legal_move(const Move& move) const {
        if (board.side_to_move == Color::WHITE) {
            BoardState<Color::WHITE, LegalPolicy> state(board);
            return bears_chess::is_legal_move(board, state, move);
        } else {
            BoardState<Color::BLACK, LegalPolicy> state(board);
            return bears_chess::is_legal_move(board, state, move);
        }
    }

    int halfmove_clock() const {
        return board.halfmove_clock;
    }

    int64_t hash() const {
        return static_cast<uint64_t>(board.hash);
    }

    std::string side_to_move() const {
        return board.side_to_move == Color::WHITE ? "w" : "b";
    }

    const Board& c_board() const {
        return board;
    }

    std::span<const ZobristHash> history() const {
        return hashes;
    }

    Move parse_move(const std::string& move_str) const {
        return convert_uci_to_move(move_str, board);
    }

private:
    Board board;
    std::vector<UndoInfo> undo;
    std::vector<ZobristHash> hashes;
};

} // namespace bears_chess_py