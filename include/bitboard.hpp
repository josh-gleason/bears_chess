#pragma once

#include "types.hpp"

namespace bears_chess {

enum class Bitboard : uint64_t {
    EMPTY = 0ULL,
    FULL = 0xffffffffffffffffULL,

    FILE_A = 0x0101010101010101ULL,
    FILE_H = 0x8080808080808080ULL,
    RANK_1 = 0x00000000000000ffULL,
    RANK_8 = 0xff00000000000000ULL,

    SQUARE_A1 = 0x0000000000000001ULL,
    SQUARE_H1 = 0x0000000000000080ULL,
    SQUARE_A8 = 0x0100000000000000ULL,
    SQUARE_H8 = 0x8000000000000000ULL
};

constexpr Bitboard to_bb(uint64_t value) noexcept {
    return static_cast<Bitboard>(value);
}

template<> struct enum_traits<Bitboard> :
    bitmask_ops,
    shift_ops,
    arithmetic_ops,
    inequality_ops,
    boolean_ops
{};

constexpr std::array<Bitboard, num_of<Square>> BB_SQUARE = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square square : iter<Square>) {
        table[idx(square)] = Bitboard::SQUARE_A1 << idx(square);
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<File>> BB_FILE = []() {
    std::array<Bitboard, num_of<File>> table{};
    for (File file : iter<File>) {
        table[idx(file)] = Bitboard::FILE_A << idx(file);
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Rank>> BB_RANK = []() {
    std::array<Bitboard, num_of<Rank>> table{};
    for (Rank rank : iter<Rank>) {
        table[idx(rank)] = Bitboard::RANK_1 << (num_of<File> * idx(rank));
    }
    return table;
}();

constexpr std::array<std::array<Bitboard, num_of<File>>, num_of<Rank>> BB_FILE_RANK = []() {
    std::array<std::array<Bitboard, num_of<File>>, num_of<Rank>> table{};
    for (Rank rank : iter<Rank>) {
        for (File file : iter<File>) {
            table[idx(file)][idx(rank)] = (BB_FILE[idx(file)] & BB_RANK[idx(rank)]);
        }
    }
    return table;
}();

constexpr Bitboard bb_square(Square square) noexcept { return BB_SQUARE[idx(square)]; }
constexpr Bitboard bb_file(File file) noexcept { return BB_FILE[idx(file)]; }
constexpr Bitboard bb_rank(Rank rank) noexcept { return BB_RANK[idx(rank)]; }
constexpr Bitboard bb_file_rank(File file, Rank rank) noexcept { return BB_FILE_RANK[idx(file)][idx(rank)]; }

constexpr Bitboard operator&(Bitboard bb, Square square) noexcept { return (bb & bb_square(square)); }
constexpr Bitboard operator|(Bitboard bb, Square square) noexcept { return (bb | bb_square(square)); }
constexpr Bitboard operator^(Bitboard bb, Square square) noexcept { return (bb ^ bb_square(square)); }
constexpr Bitboard operator&(Square square, Bitboard bb) noexcept { return (bb & square); }
constexpr Bitboard operator|(Square square, Bitboard bb) noexcept { return (bb | square); }
constexpr Bitboard operator^(Square square, Bitboard bb) noexcept { return (bb ^ square); }

constexpr Bitboard operator&(Bitboard bb, Rank rank) noexcept { return (bb & bb_rank(rank)); }
constexpr Bitboard operator|(Bitboard bb, Rank rank) noexcept { return (bb | bb_rank(rank)); }
constexpr Bitboard operator^(Bitboard bb, Rank rank) noexcept { return (bb ^ bb_rank(rank)); }
constexpr Bitboard operator&(Rank rank, Bitboard bb) noexcept { return (bb & rank); }
constexpr Bitboard operator|(Rank rank, Bitboard bb) noexcept { return (bb | rank); }
constexpr Bitboard operator^(Rank rank, Bitboard bb) noexcept { return (bb ^ rank); }

constexpr Bitboard operator&(Bitboard bb, File file) noexcept { return (bb & bb_file(file)); }
constexpr Bitboard operator|(Bitboard bb, File file) noexcept { return (bb | bb_file(file)); }
constexpr Bitboard operator^(Bitboard bb, File file) noexcept { return (bb ^ bb_file(file)); }
constexpr Bitboard operator&(File file, Bitboard bb) noexcept { return (bb & file); }
constexpr Bitboard operator|(File file, Bitboard bb) noexcept { return (bb | file); }
constexpr Bitboard operator^(File file, Bitboard bb) noexcept { return (bb ^ file); }

constexpr Square bitscan_forward(Bitboard bb) {
    // return index of the lsb that is set, undefined for bb=0
    return static_cast<Square>(__builtin_ctzll(idx(bb)));
}

constexpr Square bitscan_reverse(Bitboard bb) {
    // return index of the msb that is set, undefined for bb=0
    return static_cast<Square>(63 - __builtin_clzll(idx(bb)));
}

constexpr int popcount(Bitboard bb) {
    return __builtin_popcountll(idx(bb));
}

constexpr Bitboard lsb(Bitboard bb) noexcept {
    return bb & -bb;
}

class bb_iterator {
    public:
        constexpr bb_iterator(Bitboard _bb) noexcept : bb(_bb) {}

        constexpr bool operator!=(const bb_iterator& other) const noexcept {
            return bb != other.bb;
        }

        constexpr Square operator*() const noexcept {
            return bitscan_forward(bb);
        }

        constexpr bb_iterator& operator++() noexcept {
            bb &= bb - to_bb(1ULL);
            return *this;
        }

    private:
        Bitboard bb;
};

class bb_scan {
    public:
        constexpr bb_scan(Bitboard _bb) noexcept : bb(_bb) {}

        constexpr bb_iterator begin() const noexcept {
            return bb_iterator(bb);
        }

        constexpr bb_iterator end() const noexcept {
            return bb_iterator(Bitboard::EMPTY);
        }

    private:
        Bitboard bb;
};

} // namespace bears_chess
