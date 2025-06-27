#pragma once

#include <ostream>
#include <iostream>
#include "board.hpp"

namespace bears_chess {

constexpr const char* INITIAL_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr const char* PERFT_POSITION_2_FEN = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
constexpr const char* PERFT_POSITION_3_FEN = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr const char* PERFT_POSITION_4_FEN = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr const char* PERFT_POSITION_5_FEN = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr const char* PERFT_POSITION_6_FEN = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

Board load_fen(const std::string& fen);

std::string get_fen(const Board& board);

enum class BoardFormat : long {
    ORIENT_BLACK = 0x1,
    HIDE_LABELS = 0x2,
    HIDE_FULLMOVE_NUMBER = 0x4,
    HIDE_TURN = 0x8,
    HIDE_CASTLING_RIGHTS = 0x10,
    HIDE_HALFMOVE_CLOCK = 0x20,
    HIDE_EP_SQUARE = 0x40,
    HIDE_FEN = 0x80,
    NO_UNICODE = 0x100,
    NO_COLOR = 0x200,
    HIDE_BOARD = 0x400,
    OCCUPANCY_ONLY = 0x800, // replace all pieces with X

    RESET = 0x0,            // reset the format
    ONLY_BOARD = 0xfc,      // hide all state info and fen string
    ONLY_FEN = 0x47c,       // only print the FEN string
    ONLY_STATE = 0x480,     // only print the state information (turn, castling rights, etc...)
    HIDE_STATE = 0x7c,      // hide all state info (including turn and move number)
    SIMPLE_STATE = 0x70,    // show only turn and move number state
    PLAIN_TEXT = 0x300,     // show board in plain text (no color or unicode)

    NONE = 0x0
};

template<> struct enum_traits<BoardFormat> :
    bitmask_ops,
    flag_ops
{};

namespace detail {
    struct BoardFormatFlags {
        BoardFormat flags;
    };

    struct BitboardFormatFlags {
        Piece piece;
        Color color;
    };

    template<typename T>
    struct Highlighted {
        T obj;
        Bitboard highlights;
    };
}

inline detail::BoardFormatFlags set_board_format(BoardFormat fmt) {
    return detail::BoardFormatFlags{fmt};
}

inline detail::BitboardFormatFlags set_bitboard_piece(Piece piece, Color color = Color::BLACK) {
    return detail::BitboardFormatFlags{piece, color};
}

inline detail::Highlighted<Board> show_highlights(Board obj, Bitboard highlights) {
    return detail::Highlighted<Board>{obj, highlights};
}

inline detail::Highlighted<Bitboard> show_highlights(Bitboard obj, Bitboard highlights) {
    return detail::Highlighted<Bitboard>{obj, highlights};
}

std::ostream& operator<<(std::ostream& out, detail::BoardFormatFlags fmt);
std::ostream& operator<<(std::ostream& out, detail::BitboardFormatFlags fmt);

std::ostream& operator<<(std::ostream& out, const Board& board);
std::ostream& operator<<(std::ostream& out, Bitboard bb);
std::ostream& operator<<(std::ostream& out, const detail::Highlighted<Board>& highlighted_board);
std::ostream& operator<<(std::ostream& out, const detail::Highlighted<Bitboard>& highlighted_bitboard);

std::ostream& operator<<(std::ostream& out, Square square);
std::ostream& operator<<(std::ostream& out, Rank rank);
std::ostream& operator<<(std::ostream& out, File file);
std::ostream& operator<<(std::ostream& out, Piece piece);
std::ostream& operator<<(std::ostream& out, Color color);

} // namespace bears_chess
