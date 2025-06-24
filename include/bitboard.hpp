#pragma once

#include "types.hpp"

constexpr std::array<Bitboard, num_of<Square>> BB_SQUARE = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (int i = 0; i < num_of<Square>; ++i) {
        table[i] = 1ULL << i;
    }
    return table;
}();

constexpr Bitboard bb_square(Square square) {
    return BB_SQUARE[idx(square)];
}

constexpr Bitboard bb_square(File file, Rank rank) {
    return bb_square(square_of(file, rank));
}

constexpr Bitboard bb_file(File file) {
    return 0x0101010101010101ULL << idx(file);
}

constexpr Bitboard bb_rank(Rank rank) {
    return 0xff << (num_of<File> * idx(rank));
}

constexpr Bitboard operator&(Bitboard bb, Square square) {
    return (bb & bb_square(square));
}

constexpr Bitboard operator|(Bitboard bb, Square square) {
    return (bb | bb_square(square));
}

constexpr Bitboard operator^(Bitboard bb, Square square) {
    return (bb ^ bb_square(square));
}

constexpr Bitboard operator&(Square square, Bitboard bb) {
    return (bb & square);
}

constexpr Bitboard operator|(Square square, Bitboard bb) {
    return (bb | square);
}

constexpr Bitboard operator^(Square square, Bitboard bb) {
    return (bb ^ square);
}

constexpr Bitboard operator&(Bitboard bb, Rank rank) {
    return (bb & bb_rank(rank));
}

constexpr Bitboard operator|(Bitboard bb, Rank rank) {
    return (bb | bb_rank(rank));
}

constexpr Bitboard operator^(Bitboard bb, Rank rank) {
    return (bb ^ bb_rank(rank));
}

constexpr Bitboard operator&(Rank rank, Bitboard bb) {
    return (bb & rank);
}

constexpr Bitboard operator|(Rank rank, Bitboard bb) {
    return (bb | rank);
}

constexpr Bitboard operator^(Rank rank, Bitboard bb) {
    return (bb ^ rank);
}

constexpr Bitboard operator&(Bitboard bb, File file) {
    return (bb & bb_file(file));
}

constexpr Bitboard operator|(Bitboard bb, File file) {
    return (bb | bb_file(file));
}

constexpr Bitboard operator^(Bitboard bb, File file) {
    return (bb ^ bb_file(file));
}

constexpr Bitboard operator&(File file, Bitboard bb) {
    return (bb & file);
}

constexpr Bitboard operator|(File file, Bitboard bb) {
    return (bb | file);
}

constexpr Bitboard operator^(File file, Bitboard bb) {
    return (bb ^ file);
}

