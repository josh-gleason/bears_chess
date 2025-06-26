#pragma once

#include "types.hpp"

namespace bears_chess {

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

constexpr Square bitscan_forward(Bitboard bb) {
    // return index of the lsb that is set, undefined for bb=0
    return static_cast<Square>(__builtin_ctzll(bb));
}

constexpr Square bitscan_reverse(Bitboard bb) {
    // return index of the msb that is set, undefined for bb=0
    return static_cast<Square>(63 - __builtin_clzll(bb));
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
            bb &= bb - 1;
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
            return bb_iterator(0);
        }

    private:
        Bitboard bb;
};

} // namespace bears_chess
