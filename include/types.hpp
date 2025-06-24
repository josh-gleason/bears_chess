#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cassert>
#include <array>
#include "macros.hpp"

using Bitboard = uint64_t;

template<typename E>
constexpr std::underlying_type_t<E> idx(E e) noexcept {
    static_assert(std::is_enum_v<E>, "idx only works with enums");
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class Piece : int8_t {
    KNIGHT, BISHOP, ROOK, QUEEN, KING, PAWN,
    NONE,
    FIRST = KNIGHT, LAST = PAWN,
    LB = -1, UB = 6
};

constexpr size_t PIECE_COUNT = 6;

ENABLE_PREINCREMENT(Piece)
ENABLE_INEQUALITY(Piece)

constexpr std::array<Piece, PIECE_COUNT> PIECES = []() {
    std::array<Piece, PIECE_COUNT> table{};
    for (Piece piece = Piece::FIRST; piece <= Piece::LAST; ++piece) {
        table[idx(piece)] = piece;
    }
    return table;
}();

constexpr std::array<Piece, PIECE_COUNT> PIECES_REVERSED = []() {
    std::array<Piece, PIECE_COUNT> table{};
    for (Piece piece = Piece::LAST; piece >= Piece::FIRST; --piece) {
        table[idx(piece)] = piece;
    }
    return table;
}();

// constexpr auto BB_SQUARE = []() {
//     std::array<Bitboard, 64> table{};
//     for (int i = 0; i < 64; ++i) {
//         table[i] = 1ULL << i;
//     }
//     return table;
// }();

enum class Square : int8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NONE, FIRST = A1, LAST = H8, LB = -1, UB = 64
};

constexpr size_t SQUARE_COUNT = 64;

ENABLE_PREINCREMENT(Square)
ENABLE_INEQUALITY(Square)

enum class Rank : int8_t {
    _1, _2, _3, _4, _5, _6, _7, _8,
    NONE, FIRST = _1, LAST = _8, LB = -1, UB = 8
};

constexpr size_t RANK_COUNT = 8;

ENABLE_PREINCREMENT(Rank)
ENABLE_INEQUALITY(Rank)

enum class File : int8_t {
    A, B, C, D, E, F, G, H,
    NONE, FIRST = A, LAST = H, LB = -1, UB = 8
};

constexpr size_t FILE_COUNT = 8;

ENABLE_PREINCREMENT(File)
ENABLE_INEQUALITY(File)

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

ENABLE_BITMASK_OPERATORS(CastlingRights)
ENABLE_FLAGS(CastlingRights)

enum class Color : int8_t {
    WHITE, BLACK,
    NONE, FIRST = WHITE, LAST = BLACK, LB = -1, UB = 2
};

constexpr size_t COLOR_COUNT = 2;

ENABLE_PREINCREMENT(Color)
ENABLE_INEQUALITY(Color)

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

constexpr size_t MOVE_TYPE_COUNT = 16;

ENABLE_BITMASK_OPERATORS(MoveType);

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
