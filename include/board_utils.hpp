#pragma once

// #include <format>
#include <ostream>
#include <sstream>

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
    ORIENT_TURN = 0x1,
    ORIENT_BLACK = 0x2,
    HIDE_LABELS = 0x4,
    HIDE_FULLMOVE_NUMBER = 0x8,
    HIDE_TURN = 0x10,
    HIDE_CASTLING_RIGHTS = 0x20,
    HIDE_HALFMOVE_CLOCK = 0x40,
    HIDE_EP_SQUARE = 0x80,
    HIDE_FEN = 0x100,
    NO_UNICODE = 0x200,
    NO_COLOR = 0x400,
    HIDE_BOARD = 0x800,
    OCCUPANCY_ONLY = 0x1000, // replace all pieces with X

    RESET = 0x0,             // reset the format
    ONLY_BOARD = 0x1f8,      // hide all state info and fen string
    ONLY_FEN = 0x8f8,        // only print the FEN string
    ONLY_STATE = 0x900,      // only print the state information (turn, castling rights, etc...)
    HIDE_STATE = 0xf8,       // hide all state info (including turn and move number)
    SIMPLE_STATE = 0xe0,     // show only turn and move number state
    PLAIN_TEXT = 0x600,      // show board in plain text (no color or unicode)

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

    std::ostream& operator<<(std::ostream& out, BoardFormatFlags fmt);
    std::ostream& operator<<(std::ostream& out, BitboardFormatFlags fmt);
    std::ostream& operator<<(std::ostream& out, const Highlighted<Board>& highlighted_board);
    std::ostream& operator<<(std::ostream& out, const Highlighted<Bitboard>& highlighted_bitboard);

    template<typename T>
    concept SimplePrintable =
        std::same_as<T, Square> 
        || std::same_as<T, Rank> 
        || std::same_as<T, File> 
        || std::same_as<T, Piece> 
        || std::same_as<T, Color> 
        || std::same_as<T, MoveType>;
    
    template<typename T>
    concept BoardPrintable =
        std::same_as<T, bears_chess::Board>
        || std::same_as<T, Highlighted<Board>>;
    
    template<typename T>
    concept BitboardPrintable =
        std::same_as<T, bears_chess::Bitboard>
        || std::same_as<T, Highlighted<Bitboard>>;
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

std::ostream& operator<<(std::ostream& out, const Board& board);
std::ostream& operator<<(std::ostream& out, Bitboard bb);

std::ostream& operator<<(std::ostream& out, Square square);
std::ostream& operator<<(std::ostream& out, Rank rank);
std::ostream& operator<<(std::ostream& out, File file);
std::ostream& operator<<(std::ostream& out, Piece piece);
std::ostream& operator<<(std::ostream& out, Color color);
std::ostream& operator<<(std::ostream& out, MoveType move_type);
std::ostream& operator<<(std::ostream& out, Move move);

File parse_file(char f);
Rank parse_rank(char r);
Square parse_square(char f, char r);
Piece parse_piece(char p);
Move parse_uci_move(const std::string& s, const Board& board);

} // namespace bears_chess

template<bears_chess::detail::BoardPrintable BoardT>
struct std::formatter<BoardT> {
    bears_chess::detail::BoardFormatFlags fmt;

    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        using namespace bears_chess;
        auto it = ctx.begin();
        fmt.flags = BoardFormat::NONE;
        for (;it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
                case '<': fmt.flags |= BoardFormat::NONE; break;          // orient white (default)
                case '+': fmt.flags |= BoardFormat::ORIENT_TURN; break;
                case '>': fmt.flags |= BoardFormat::ORIENT_BLACK; break;
                case 'd': fmt.flags |= BoardFormat::HIDE_LABELS; break;   // hide board coorDinates
                case 's': fmt.flags |= BoardFormat::HIDE_STATE; break;
                case 'S': fmt.flags |= BoardFormat::ONLY_STATE; break;    // equivalent to of
                case 'o': fmt.flags |= BoardFormat::HIDE_BOARD; break;
                case 'O': fmt.flags |= BoardFormat::ONLY_BOARD; break;    // equivalent to sf
                case 'f': fmt.flags |= BoardFormat::HIDE_FEN; break;
                case 'F': fmt.flags |= BoardFormat::ONLY_FEN; break;      // equivalent to so
                // support terminals with limited rending support
                case 'a': fmt.flags |= BoardFormat::NO_UNICODE; break;    // ASCII only
                case 'c': fmt.flags |= BoardFormat::NO_COLOR; break;      // no ansi color sequences
                // fine grained control of visible state (irrelevant if state completely hidden)
                case '#': fmt.flags |= BoardFormat::HIDE_FULLMOVE_NUMBER; break;
                case 'm': fmt.flags |= BoardFormat::HIDE_TURN; break;
                case 'r': fmt.flags |= BoardFormat::HIDE_CASTLING_RIGHTS; break;
                case 'h': fmt.flags |= BoardFormat::HIDE_HALFMOVE_CLOCK; break;
                case 'e': fmt.flags |= BoardFormat::HIDE_EP_SQUARE; break;
                // show pieces as X instead of actual piece name
                case 'X': fmt.flags |= BoardFormat::OCCUPANCY_ONLY; break;
                default:
                    throw std::format_error(std::string("Invalid Board format specifier: '") + *it + "'");
            }
        }
        return it;
    }

    template<typename FormatContext>
    auto format(const BoardT &b, FormatContext& ctx) const {
        return std::ranges::copy(std::move(ostringstream() << fmt << b).str(), ctx.out()).out;
    }
};


template<bears_chess::detail::BitboardPrintable BitboardT>
struct std::formatter<BitboardT> {
    bears_chess::detail::BoardFormatFlags fmt;
    bears_chess::detail::BitboardFormatFlags bb_fmt;

    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        using namespace bears_chess;
        auto it = ctx.begin();
        fmt.flags = BoardFormat::NONE;
        bb_fmt = {Piece::NONE, Color::BLACK};
        for (;it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
                case '<': fmt.flags |= BoardFormat::NONE; break;          // orient white (default)
                case '>': fmt.flags |= BoardFormat::ORIENT_BLACK; break;
                case 'd': fmt.flags |= BoardFormat::HIDE_LABELS; break;   // hide board coorDinates
                case 'o': fmt.flags |= BoardFormat::HIDE_BOARD; break;
                case 'O': fmt.flags |= BoardFormat::ONLY_BOARD; break;    // equivalent to sf
                case 'f': fmt.flags |= BoardFormat::HIDE_FEN; break;
                case 'F': fmt.flags |= BoardFormat::ONLY_FEN; break;      // equivalent to so
                // support terminals with limited rending support
                case 'a': fmt.flags |= BoardFormat::NO_UNICODE; break;    // ASCII only
                case 'c': fmt.flags |= BoardFormat::NO_COLOR; break;      // no ansi color sequences
                // types of pieces to show
                case 'N': bb_fmt = {Piece::KNIGHT, Color::BLACK}; break;
                case 'B': bb_fmt = {Piece::BISHOP, Color::BLACK}; break;
                case 'R': bb_fmt = {Piece::ROOK, Color::BLACK}; break;
                case 'Q': bb_fmt = {Piece::QUEEN, Color::BLACK}; break;
                case 'K': bb_fmt = {Piece::KING, Color::BLACK}; break;
                case 'P': bb_fmt = {Piece::PAWN, Color::BLACK}; break;
                case 'X': bb_fmt = {Piece::NONE, Color::BLACK}; break;
                case 'n': bb_fmt = {Piece::KNIGHT, Color::WHITE}; break;
                case 'b': bb_fmt = {Piece::BISHOP, Color::WHITE}; break;
                case 'r': bb_fmt = {Piece::ROOK, Color::WHITE}; break;
                case 'q': bb_fmt = {Piece::QUEEN, Color::WHITE}; break;
                case 'k': bb_fmt = {Piece::KING, Color::WHITE}; break;
                case 'p': bb_fmt = {Piece::PAWN, Color::WHITE}; break;
                case 'x': bb_fmt = {Piece::NONE, Color::WHITE}; break;
                default:
                    throw std::format_error(string("Invalid Bitboard format specifier: '") + *it + "'");
            }
        }
        return it;
    }

    template<typename FormatContext>
    FormatContext::iterator format(const BitboardT& bb, FormatContext& ctx) const {
        return std::ranges::copy(std::move(ostringstream() << fmt << bb_fmt << bb).str(), ctx.out()).out;
    }
};

template<bears_chess::detail::SimplePrintable T>
struct std::formatter<T>
{
    bears_chess::detail::BoardFormatFlags fmt;

    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        using namespace bears_chess;
        auto it = ctx.begin();
        fmt.flags = BoardFormat::NONE;
        for (;it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
                case 'a': fmt.flags |= BoardFormat::NO_UNICODE; break;    // ASCII only
                case 'c': fmt.flags |= BoardFormat::NO_COLOR; break;      // no ansi color sequences
                default:
                    throw std::format_error(string("Invalid format specifier: '") + *it + "'");
            }
        }
        return it;
    }

    template<typename FormatContext>
    FormatContext::iterator format(const T &t, FormatContext& ctx) const {
        return std::ranges::copy(std::move(ostringstream() << fmt << t).str(), ctx.out()).out;
    }
};

template <>
struct std::formatter<bears_chess::Move>
{
    enum NotationType {
        VERBOSE,
        FULL,
        // ALGEBRAIC, // not implemented yet
    };

    bears_chess::detail::BoardFormatFlags fmt;
    NotationType notation = VERBOSE;

    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        using namespace bears_chess;
        auto it = ctx.begin();
        fmt.flags = BoardFormat::NONE;
        for (;it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
                case 'a': fmt.flags |= BoardFormat::NO_UNICODE; break;    // ASCII only
                case 'c': fmt.flags |= BoardFormat::NO_COLOR; break;      // no ansi color sequences
                // case 's': notation = ALGEBRAIC; break;
                case 'f': notation = FULL; break;
                case 'v': notation = VERBOSE; break;
                default:
                    throw std::format_error(string("Invalid format specifier: '") + *it + "'");
            }
        }
        return it;
    }

    template<typename FormatContext>
    FormatContext::iterator format(const bears_chess::Move &m, FormatContext& ctx) const {
        ostringstream sout;
        if (notation == FULL) {
            sout << fmt << m.from << m.to;
            if (is_promotion(m.move_type)) {
                sout << promote_to(m.move_type);
            }
        } else {
            sout << fmt << m;
        }
        return std::ranges::copy(std::move(sout).str(), ctx.out()).out;
    }
};
