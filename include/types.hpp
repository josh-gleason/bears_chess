#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cassert>
#include <array>
#include "enum_traits.hpp"

namespace bears_chess {

template<typename E>
constexpr std::underlying_type_t<E> idx(E e) noexcept {
    static_assert(std::is_enum_v<E>, "idx only works with enums");
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class Piece : int8_t {
    KNIGHT, BISHOP, ROOK, QUEEN, KING, PAWN,
    NONE
};

template<> struct enum_traits<Piece> :
    preincrement_ops,
    inequality_ops,
    range_ops<Piece::KNIGHT, Piece::PAWN>
{};

constexpr std::array<Piece, num_of<Piece>> PIECES = []() {
    std::array<Piece, num_of<Piece>> table{};
    for (Piece piece : iter<Piece>) {
        table[idx(piece)] = piece;
    }
    return table;
}();

constexpr std::array<Piece, num_of<Piece>> PIECES_REVERSED = []() {
    std::array<Piece, num_of<Piece>> table{};
    for (Piece piece : iter_rev<Piece>) {
        table[idx(piece)] = piece;
    }
    return table;
}();

enum class Square : int8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NONE
};

template<> struct enum_traits<Square> :
    preincrement_ops,
    inequality_ops,
    range_ops<Square::A1, Square::H8>
{};

enum class Rank : int8_t {
    _1, _2, _3, _4, _5, _6, _7, _8,
    NONE
};

template<> struct enum_traits<Rank> :
    preincrement_ops,
    inequality_ops,
    range_ops<Rank::_1, Rank::_8>
{};

enum class File : int8_t {
    A, B, C, D, E, F, G, H,
    NONE
};

template<> struct enum_traits<File> :
    preincrement_ops,
    inequality_ops,
    range_ops<File::A, File::H>
{};

constexpr Rank rank_of(Square square) {
    return static_cast<Rank>(idx(square) >> 3);
}

constexpr File file_of(Square square) {
    return static_cast<File>(idx(square) & 0x7);
}

constexpr Square square_of(File file, Rank rank) {
    return static_cast<Square>((idx(rank) << 3) | idx(file));
}

constexpr bool is_light(Square square) {
    return (idx(rank_of(square)) + idx(square)) % 2 == 1;
}

enum class CastlingRights : uint8_t {
    NONE = 0b0000,
    WHITE_KING = 0b0001,
    WHITE_QUEEN = 0b0010,
    BLACK_KING = 0b0100,
    BLACK_QUEEN = 0b1000,
    WHITE = 0b0011,
    BLACK = 0b1100,
    KINGS = 0b0101,
    QUEENS = 0b1010,
    ALL = 0b1111
};

template<> struct enum_traits<CastlingRights> :
    bitmask_ops,
    shift_ops,
    flag_ops
{};

enum class Color : int8_t {
    WHITE, BLACK,
    NONE
};

template<> struct enum_traits<Color> :
    preincrement_ops,
    inequality_ops,
    range_ops<Color::WHITE, Color::BLACK>
{};

constexpr Color operator~(Color color) noexcept {
    return static_cast<Color>(static_cast<int8_t>(color) ^ 1);
}

constexpr CastlingRights clear_castling_rights(CastlingRights r, Color c) {
    if (c == Color::WHITE) {
        return r & CastlingRights::BLACK;
    } else {
        return r & CastlingRights::WHITE;
    }
}

template<Piece castle_side>
constexpr CastlingRights clear_half_castling_rights(CastlingRights r, Color c) {
    if constexpr (castle_side == Piece::KING) {
        if (c == Color::WHITE) {
            return clear_flag(r, CastlingRights::WHITE_KING);
        } else {
            return clear_flag(r, CastlingRights::BLACK_KING);
        }
    } else {
        if (c == Color::WHITE) {
            return clear_flag(r, CastlingRights::WHITE_QUEEN);
        } else {
            return clear_flag(r, CastlingRights::BLACK_QUEEN);
        }
    }
    
}

template<Piece side>
constexpr bool castling_allowed(CastlingRights castling_rights, Color color) {
    static_assert(side == Piece::KING || side == Piece::QUEEN, "only king or queen side castling possible");
    if constexpr (side == Piece::KING) {
        return check_flag(castling_rights & CastlingRights::KINGS, CastlingRights::WHITE << (idx(color) * 2));
    } else {
        return check_flag(castling_rights & CastlingRights::QUEENS, CastlingRights::WHITE << (idx(color) * 2));
    }
}

enum class MoveType : int8_t {
    QUIET = 0b0000,
    DOUBLE_PAWN_PUSH = 0b0001,
    KING_CASTLE = 0b0010,
    QUEEN_CASTLE = 0b0011,
    CAPTURE = 0b0100,
    EP_CAPTURE = 0b0101,
    // numbers not sequential, do not iterate
    KNIGHT_PROMOTION = 0b1000,
    BISHOP_PROMOTION = 0b1001,
    ROOK_PROMOTION = 0b1010,
    QUEEN_PROMOTION = 0b1011,
    KNIGHT_PROMOTION_CAPTURE = 0b1100,
    BISHOP_PROMOTION_CAPTURE = 0b1101,
    ROOK_PROMOTION_CAPTURE = 0b1110,
    QUEEN_PROMOTION_CAPTURE = 0b1111,
    NONE = 16,

    CAPTURE_BIT = 0b0100,
    PROMOTION_BIT = 0b1000,
    PROMOTION_PIECE_BITS = 0b0011,
};

template<> struct enum_traits<MoveType> :
    bitmask_ops,
    flag_ops
{};

constexpr bool is_capture(MoveType move_type) {
    return static_cast<bool>(move_type & MoveType::CAPTURE_BIT);
}

constexpr bool is_promotion(MoveType move_type) {
    return static_cast<bool>(move_type & MoveType::PROMOTION_BIT);
}

constexpr Piece promote_to(MoveType move_type) {
    assert(is_promotion(move_type));
    return static_cast<Piece>(move_type & MoveType::PROMOTION_PIECE_BITS);
}

struct Move {
    Square from;
    Square to;
    MoveType move_type;
};

template<Piece castle_side>
constexpr auto CASTLE_MOVES = []() -> std::array<Move, num_of<Color>> {
    static_assert(castle_side == Piece::KING || castle_side == Piece::QUEEN);
    if constexpr (castle_side == Piece::KING) {
        return {
            Move{Square::E1, Square::G1, MoveType::KING_CASTLE},
            Move{Square::E8, Square::G8, MoveType::KING_CASTLE}
        };
    } else {
        return {
            Move{Square::E1, Square::C1, MoveType::QUEEN_CASTLE},
            Move{Square::E8, Square::C8, MoveType::QUEEN_CASTLE}
        };
    }
}();

template<Piece castle_side>
constexpr auto CASTLE_ROOK_FROM_SQUARES = []() -> std::array<Square, num_of<Color>> {
    static_assert(castle_side == Piece::KING || castle_side == Piece::QUEEN);
    if constexpr (castle_side == Piece::KING) {
        return { Square::H1, Square::H8 };
    } else {
        return { Square::A1, Square::A8 };
    }
}();

template<Piece castle_side>
constexpr auto CASTLE_ROOK_TO_SQUARES = []() -> std::array<Square, num_of<Color>> {
    static_assert(castle_side == Piece::KING || castle_side == Piece::QUEEN);
    if constexpr (castle_side == Piece::KING) {
        return { Square::F1, Square::F8 };
    } else {
        return { Square::D1, Square::D8 };
    }
}();

struct UndoInfo {
    Move move;
    Piece captured;
    CastlingRights castling_rights;
    Square ep_square;
    uint8_t halfmove_clock;
};

enum class Direction : int8_t {
    NORTH = 8,
    EAST = 1,
    SOUTH = -8,
    WEST = -1,
    NORTHEAST = NORTH + EAST,
    SOUTHEAST = SOUTH + EAST,
    SOUTHWEST = SOUTH + WEST,
    NORTHWEST = NORTH + WEST
};

namespace detail {
    constexpr size_t dir_hash_fun(const Direction& d) noexcept {
        return static_cast<size_t>((static_cast<int8_t>(d) + 9) % 11);
    }
}

template<> struct enum_traits<Direction> :
    hash_ops<Direction, detail::dir_hash_fun, 11>
{};

constexpr std::array<Direction, 4> CARDINAL_DIRECTIONS = {
    Direction::NORTH, Direction::EAST, Direction::SOUTH, Direction::WEST
};

constexpr std::array<Direction, 4> ORDINAL_DIRECTIONS = {
    Direction::NORTHEAST, Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::NORTHWEST
};

template<int times = 1>
constexpr Square sq_shift(Square sq, Direction dir) {
    // no bounds checking
    if constexpr (times == 1) {
        return static_cast<Square>(idx(sq) + idx(dir));
    } else {
        return static_cast<Square>(idx(sq) + (idx(dir) * times));
    }
}

constexpr Square captured_ep_square(Color capturing_side, Square ep_square) {
    // return the square with the en passant captured piece on it
    return sq_shift(ep_square, static_cast<Direction>((idx(capturing_side) << 4) - 8));
}

constexpr Square double_push_ep_square(Color side_to_move, Square to) {
    // return the en passant square due to a double pawn push
    return sq_shift(to, static_cast<Direction>((idx(side_to_move) << 4) - 8));
}


using MoveList = std::vector<Move>;

template<typename T, size_t N>
constexpr bool all_unique(const std::array<T, N>& arr) {
    for (size_t i = 0; i < N; ++i)
        for (size_t j = i + 1; j < N; ++j)
            if (arr[i] == arr[j])
                return false;
    return true;
}

} // namespace bears_chess
