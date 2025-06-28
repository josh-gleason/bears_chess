#pragma once

#include "types.hpp"
#include <bit>

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

constexpr std::array<Bitboard, num_of<Square> + 1> BB_SQUARE = []() {
    std::array<Bitboard, num_of<Square> + 1> table{};
    for (Square square : iter<Square>) {
        table[idx(square)] = Bitboard::SQUARE_A1 << idx(square);
    }
    table[idx(Square::NONE)] = Bitboard::EMPTY;
    return table;
}();

constexpr std::array<Bitboard, num_of<File> + 1> BB_FILE = []() {
    std::array<Bitboard, num_of<File> + 1> table{};
    for (File file : iter<File>) {
        table[idx(file)] = Bitboard::FILE_A << idx(file);
    }
    table[idx(File::NONE)] = Bitboard::EMPTY;
    return table;
}();

constexpr std::array<Bitboard, num_of<Rank> + 1> BB_RANK = []() {
    std::array<Bitboard, num_of<Rank> + 1> table{};
    for (Rank rank : iter<Rank>) {
        table[idx(rank)] = Bitboard::RANK_1 << (num_of<File> * idx(rank));
    }
    table[idx(Rank::NONE)] = Bitboard::EMPTY;
    return table;
}();

constexpr std::array<std::array<Bitboard, num_of<Rank> + 1>, num_of<File> + 1> BB_FILE_RANK = []() {
    std::array<std::array<Bitboard, num_of<Rank> + 1>, num_of<File> + 1> table{};
    for (File file : iter<File>) {
        for (Rank rank : iter<Rank>) {
            table[idx(file)][idx(rank)] = (BB_FILE[idx(file)] & BB_RANK[idx(rank)]);
        }
        table[idx(file)][idx(Rank::NONE)] = Bitboard::EMPTY;
    }
    for (Rank rank : iter<Rank>) {
        table[idx(File::NONE)][idx(Rank::NONE)] = Bitboard::EMPTY;
    }
    table[idx(File::NONE)][idx(Rank::NONE)] = Bitboard::EMPTY;
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

template<bool Safe=false> constexpr Bitboard bb_n(Bitboard bb) noexcept {
    return bb << num_of<File>;
}

template<bool Safe=false> constexpr Bitboard bb_e(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::H);
    return bb << 1;
}

template<bool Safe=false> constexpr Bitboard bb_s(Bitboard bb) noexcept {
    return bb >> num_of<File>;
}

template<bool Safe=false> constexpr Bitboard bb_w(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::A);
    return bb >> 1;
}

template<bool Safe=false> constexpr Bitboard bb_ne(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::H);
    return bb << (num_of<File> + 1);
}

template<bool Safe=false> constexpr Bitboard bb_nw(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::A);
    return bb << (num_of<File> - 1);
}

template<bool Safe=false> constexpr Bitboard bb_se(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::H);
    return bb >> (num_of<File> - 1);
}

template<bool Safe=false> constexpr Bitboard bb_sw(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::A);
    return bb >> (num_of<File> + 1);
}

template<bool Safe=false> constexpr Bitboard bb_ww(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::A) | bb_file(File::B));
    return bb >> 2;
}

template<bool Safe=false> constexpr Bitboard bb_nww(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::A) | bb_file(File::B));
    return bb << (num_of<File> - 2);
}

template<bool Safe=false> constexpr Bitboard bb_nnww(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::A) | bb_file(File::B));
    return bb << (2 * num_of<File> - 2);
}

template<bool Safe=false> constexpr Bitboard bb_nnw(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::A);
    return bb << (2 * num_of<File> - 1);
}

template<bool Safe=false> constexpr Bitboard bb_nn(Bitboard bb) noexcept {
    return bb << (2 * num_of<File>);
}

template<bool Safe=false> constexpr Bitboard bb_nne(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::H);
    return bb << (2 * num_of<File> + 1);
}

template<bool Safe=false> constexpr Bitboard bb_nnee(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::G) | bb_file(File::H));
    return bb << (2 * num_of<File> + 2);
}

template<bool Safe=false> constexpr Bitboard bb_nee(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::G) | bb_file(File::H));
    return bb << (num_of<File> + 2);
}

template<bool Safe=false> constexpr Bitboard bb_ee(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::G) | bb_file(File::H));
    return bb << 2;
}

template<bool Safe=false> constexpr Bitboard bb_see(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::G) | bb_file(File::H));
    return bb >> (num_of<File> - 2);
}

template<bool Safe=false> constexpr Bitboard bb_ssee(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::G) | bb_file(File::H));
    return bb >> (2 * num_of<File> - 2);
}

template<bool Safe=false> constexpr Bitboard bb_sse(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::H);
    return bb >> (2 * num_of<File> - 1);
}

template<bool Safe=false> constexpr Bitboard bb_ss(Bitboard bb) noexcept {
    return bb >> (2 * num_of<File>);
}

template<bool Safe=false> constexpr Bitboard bb_ssw(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~bb_file(File::A);
    return bb >> (2 * num_of<File> + 1);
}

template<bool Safe=false> constexpr Bitboard bb_ssww(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::A) | bb_file(File::B));
    return bb >> (2 * num_of<File> + 2);
}

template<bool Safe=false> constexpr Bitboard bb_sww(Bitboard bb) noexcept {
    if constexpr (Safe)
        bb &= ~(bb_file(File::A) | bb_file(File::B));
    return bb >> (num_of<File> + 2);
}

// index of the lsb that is set, undefined for bb=0
constexpr Square bitscan_forward(Bitboard bb) noexcept { return static_cast<Square>(__builtin_ctzll(idx(bb))); }
// index of the msb that is set, undefined for bb=0
constexpr Square bitscan_reverse(Bitboard bb) noexcept { return static_cast<Square>(63 - __builtin_clzll(idx(bb))); }
// number of bits set
constexpr int popcount(Bitboard bb) noexcept { return __builtin_popcountll(idx(bb)); }
// extract the least significant bit
constexpr Bitboard lsb(Bitboard bb) noexcept { return (bb & -bb); }

class bb_square_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Square;
        using difference_type = std::ptrdiff_t;
        constexpr bb_square_iterator(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bool operator!=(const bb_square_iterator& other) const noexcept { return bb != other.bb; }
        constexpr Square operator*() const noexcept { return bitscan_forward(bb); }
        constexpr bb_square_iterator& operator++() noexcept {
            bb &= bb - to_bb(1ULL);
            return *this;
        }
    private:
        Bitboard bb;
};

// for (Square sq : bb_square_scan(bb)) to iterate over occupied squares of Bitboard bb (starting with lsb)
class bb_square_scan {
    public:
        constexpr bb_square_scan(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bb_square_iterator begin() const noexcept { return bb_square_iterator(bb); }
        constexpr bb_square_iterator end() const noexcept { return bb_square_iterator(Bitboard::EMPTY); }
    private:
        Bitboard bb;
};

class bb_bit_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Bitboard;
        using difference_type = std::ptrdiff_t;
        constexpr bb_bit_iterator(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bool operator!=(const bb_bit_iterator& other) const noexcept { return bb != other.bb; }
        constexpr Bitboard operator*() const noexcept { return lsb(bb); }
        constexpr bb_bit_iterator& operator++() noexcept {
            bb &= bb - to_bb(1ULL);
            return *this;
        }
    private:
        Bitboard bb;
};

// for (Bitboard bb_bit : bb_bit_scan(bb)) to iterate over set bits of bb (starting with lsb)
class bb_bit_scan {
    public:
        constexpr bb_bit_scan(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bb_bit_iterator begin() const noexcept { return bb_bit_iterator(bb); }
        constexpr bb_bit_iterator end() const noexcept { return bb_bit_iterator(Bitboard::EMPTY); }
    private:
        Bitboard bb;
};

constexpr std::array<Bitboard, num_of<Square>> BB_KNIGHT_MOVES = []() {
    std::array<Bitboard, num_of<Square>> masks{};
    for (Square sq : iter<Square>) {
        Bitboard center = BB_SQUARE[idx(sq)];
        masks[idx(sq)] = (
            bb_nne<true>(center) |
            bb_nee<true>(center) |
            bb_see<true>(center) |
            bb_sse<true>(center) |
            bb_ssw<true>(center) |
            bb_sww<true>(center) |
            bb_nww<true>(center) |
            bb_nnw<true>(center)
        );
    }
    return masks;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_KING_MOVES = []() {
    std::array<Bitboard, num_of<Square>> masks{};
    for (Square sq : iter<Square>) {
        Bitboard center = BB_SQUARE[idx(sq)];
        masks[idx(sq)] = (
            bb_n<true>(center) |
            bb_ne<true>(center) |
            bb_e<true>(center) |
            bb_se<true>(center) |
            bb_s<true>(center) |
            bb_sw<true>(center) |
            bb_w<true>(center) |
            bb_nw<true>(center)
        );
    }
    return masks;
}();

constexpr std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> BB_SINGLE_PAWN_MOVES = []() {
    std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> masks{};
    for (Color c : iter<Color>) {
        for (Square sq : iter<Square>) {
            Bitboard center = BB_SQUARE[idx(sq)];
            if (c == Color::WHITE) {
                masks[idx(c)][idx(sq)] = bb_n<true>(center);
            } else {
                masks[idx(c)][idx(sq)] = bb_s<true>(center);
            }
        }
    }
    return masks;
}();

constexpr std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> BB_DOUBLE_PAWN_MOVES = []() {
    std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> masks{};
    for (Color c : iter<Color>) {
        for (Square sq : iter<Square>) {
            Bitboard center = BB_SQUARE[idx(sq)];
            if (c == Color::WHITE && rank_of(sq) == Rank::_2) {
                masks[idx(c)][idx(sq)] = bb_nn<true>(center);
            } else if (c == Color::BLACK && rank_of(sq) == Rank::_7) {
                masks[idx(c)][idx(sq)] = bb_ss<true>(center);
            } else {
                masks[idx(c)][idx(sq)] = Bitboard::EMPTY;
            }
        }
    }
    return masks;
}();

constexpr std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> BB_CAPTURE_PAWN_MOVES = []() {
    std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> masks{};
    for (Color c : iter<Color>) {
        for (Square sq : iter<Square>) {
            Bitboard center = BB_SQUARE[idx(sq)];
            if (c == Color::WHITE) {
                masks[idx(c)][idx(sq)] = bb_ne<true>(center) | bb_nw<true>(center);
            } else {
                masks[idx(c)][idx(sq)] = bb_se<true>(center) | bb_sw<true>(center);
            }
        }
    }
    return masks;
}();

constexpr const Bitboard BB_PROMOTION_RANKS = bb_rank(Rank::_1) | bb_rank(Rank::_8);

} // namespace bears_chess
