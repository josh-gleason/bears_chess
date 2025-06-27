#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cassert>
#include <array>
#include "enum_traits.hpp"

namespace bears_chess {

// using Bitboard = uint64_t;

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
    ALL = 0b1111
};

template<> struct enum_traits<CastlingRights> :
    bitmask_ops,
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

struct UndoInfo {
    Move move;
    Piece captured;
    CastlingRights castling_rights;
    Square ep_square;
    uint8_t halfmove_clock;
};

using MoveList = std::vector<Move>;

} // namespace bears_chess
