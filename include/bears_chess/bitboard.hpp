#pragma once

#include "bears_chess/types.hpp"

#include <vector>
#if defined(__BMI2__) || defined(_MSC_VER)
#include <immintrin.h>
#define PEXT_SUPPORT
#endif

namespace bears_chess {

void init_bitboards();

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
    bitwise_ops,
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
        table[idx(File::NONE)][idx(rank)] = Bitboard::EMPTY;
    }
    table[idx(File::NONE)][idx(Rank::NONE)] = Bitboard::EMPTY;
    return table;
}();

constexpr Bitboard bb_square(Square square) noexcept { return BB_SQUARE[idx(square)]; }
constexpr Bitboard bb_file(File file) noexcept { return BB_FILE[idx(file)]; }
constexpr Bitboard bb_rank(Rank rank) noexcept { return BB_RANK[idx(rank)]; }
constexpr Bitboard bb_file(Square sq) noexcept { return BB_FILE[idx(file_of(sq))]; }
constexpr Bitboard bb_rank(Square sq) noexcept { return BB_RANK[idx(rank_of(sq))]; }
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

constexpr std::array<Bitboard, hash_max<Direction>> BB_DIRECTION_PREMASK = []() {
    static_assert([]{
        constexpr std::array hashes = {
            hash(Direction::NORTH), hash(Direction::EAST),
            hash(Direction::SOUTH), hash(Direction::WEST),
            hash(Direction::NORTHEAST), hash(Direction::NORTHWEST),
            hash(Direction::SOUTHEAST), hash(Direction::SOUTHWEST)
        };
        if (!all_unique(hashes)) return false;
        for (int h : hashes)
            if (h < 0 || h >= hash_max<Direction>) return false;
        return true;
    }(), "Direction hash is invalid or out of range");

    std::array<Bitboard, hash_max<Direction>> table{};
    for (int i = 0; i < hash_max<Direction>; ++i) {
        table[i] = Bitboard::FULL;    
    }
    table[hash<Direction>(Direction::EAST)] = ~bb_file(File::H);
    table[hash<Direction>(Direction::NORTHEAST)] = ~bb_file(File::H);
    table[hash<Direction>(Direction::SOUTHEAST)] = ~bb_file(File::H);
    table[hash<Direction>(Direction::WEST)] = ~bb_file(File::A);
    table[hash<Direction>(Direction::SOUTHWEST)] = ~bb_file(File::A);
    table[hash<Direction>(Direction::NORTHWEST)] = ~bb_file(File::A);
    return table;
}();

constexpr Bitboard bb_direction_premask(Direction dir) noexcept {
    return BB_DIRECTION_PREMASK[hash<Direction>(dir)];
}

template<bool Safe = false>
constexpr Bitboard bb_shift(Bitboard bb, Direction dir) noexcept {
    if constexpr (Safe) {
        bb &= bb_direction_premask(dir);
    }
    if (idx(dir) < 0) {
        return bb >> -idx(dir);
    } else {
        return bb << idx(dir);
    }
}

template<Direction dir, bool Safe=false>
constexpr Bitboard bb_shift(Bitboard bb) noexcept {
    if constexpr (Safe && (dir != Direction::SOUTH) && (dir != Direction::NORTH)) {
        bb &= bb_direction_premask(dir);
    }
    if constexpr (idx(dir) < 0) {
        return bb >> -idx(dir);
    } else {
        return bb << idx(dir);
    }
}

// TODO replace with bb_shift
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

template<bool First = false, bool Last = false>
constexpr Bitboard ray(Square from, Direction dir) noexcept {
    Bitboard bb_bit = bb_square(from);
    if constexpr (!First)
        bb_bit = bb_shift<true>(bb_bit, dir);
    Bitboard bb_prev_bit = bb_bit;
    Bitboard mask = Bitboard::EMPTY;
    while (nonzero(bb_bit)) {
        mask |= bb_bit;
        bb_prev_bit = bb_bit;
        bb_bit = bb_shift<true>(bb_bit, dir);
    }
    if constexpr (!Last)
        mask &= ~bb_prev_bit;
    return mask;
}

template<Piece move_type = Piece::QUEEN, bool first = true, bool last = false>
constexpr Bitboard ray_between(Square from, Square to) noexcept {
    if (zero(bb_slider_attack_mask<move_type, true, true>(from) & bb_square(to))) {
        return Bitboard::EMPTY;
    }
    Direction dir = dir_between<Direction>(from, to);
    Bitboard bb_bit = bb_square(from);
    if constexpr (!first) {
        bb_bit = bb_shift<true>(bb_bit, dir) & ~bb_square(to);
    }
    Bitboard mask = Bitboard::EMPTY;
    while (nonzero(bb_bit)) {
        mask |= bb_bit;
        bb_bit = bb_shift<true>(bb_bit, dir) & ~bb_square(to);
    }
    if constexpr (last) {
        mask |= bb_square(to);
    }
    return mask;
}

template<Piece move_type = Piece::QUEEN>
constexpr std::array<std::array<Bitboard, num_of<Square>>, num_of<Square>> BB_RAY = []() {
    std::array<std::array<Bitboard, num_of<Square>>, num_of<Square>> table{};
    for (Square from : iter<Square>) {
        for (Square to : iter<Square>) {
            table[idx(from)][idx(to)] = ray_between<move_type>(from, to);
        }
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_FILE_OF = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = bb_file(file_of(sq));
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_RANK_OF = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = bb_rank(rank_of(sq));
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_DIAG45_OF = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = (
            ray<true, true>(sq, Direction::NORTHEAST)
            | ray<true, true>(sq, Direction::SOUTHWEST)
        );
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_DIAG135_OF = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = (
            ray<true, true>(sq, Direction::NORTHWEST)
            | ray<true, true>(sq, Direction::SOUTHEAST)
        );
    }
    return table;
}();

template<Direction dir> requires is_ordinal<dir>
inline Bitboard get_diag_of(Square sq) {
    if constexpr (dir == Direction::NORTHEAST || dir == Direction::SOUTHWEST) {
        return BB_DIAG45_OF[idx(sq)];
    } else {
        return BB_DIAG135_OF[idx(sq)];
    }
}

// index of the lsb that is set, undefined for bb=0
constexpr Square bitscan_forward(Bitboard bb) noexcept { return static_cast<Square>(__builtin_ctzll(idx(bb))); }
// index of the msb that is set, undefined for bb=0
constexpr Square bitscan_reverse(Bitboard bb) noexcept { return static_cast<Square>(63 - __builtin_clzll(idx(bb))); }
// number of bits set
constexpr int popcount(Bitboard bb) noexcept { return __builtin_popcountll(idx(bb)); }
// extract the least significant bit
constexpr Bitboard lsb(Bitboard bb) noexcept { return (bb & -bb); }

class BBSquareIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Square;
        using difference_type = std::ptrdiff_t;
        constexpr BBSquareIterator(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bool operator!=(const BBSquareIterator& other) const noexcept { return bb != other.bb; }
        constexpr Square operator*() const noexcept { return bitscan_forward(bb); }
        constexpr BBSquareIterator& operator++() noexcept {
            bb &= bb - to_bb(1ULL);
            return *this;
        }
    private:
        Bitboard bb;
};

// for (Square sq : BBSquareScan(bb)) to iterate over occupied squares of Bitboard bb (starting with lsb)
class BBSquareScan {
    public:
        constexpr BBSquareScan(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr BBSquareIterator begin() const noexcept { return BBSquareIterator(bb); }
        constexpr BBSquareIterator end() const noexcept { return BBSquareIterator(Bitboard::EMPTY); }
    private:
        Bitboard bb;
};

class BBBitIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Bitboard;
        using difference_type = std::ptrdiff_t;
        constexpr BBBitIterator(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr bool operator!=(const BBBitIterator& other) const noexcept { return bb != other.bb; }
        constexpr Bitboard operator*() const noexcept { return lsb(bb); }
        constexpr BBBitIterator& operator++() noexcept {
            bb &= bb - to_bb(1ULL);
            return *this;
        }
    private:
        Bitboard bb;
};

// for (Bitboard bb_bit : BBBitScan(bb)) to iterate over set bits of bb (starting with lsb)
class BBBitScan {
    public:
        constexpr BBBitScan(Bitboard _bb) noexcept : bb(_bb) {}
        constexpr BBBitIterator begin() const noexcept { return BBBitIterator(bb); }
        constexpr BBBitIterator end() const noexcept { return BBBitIterator(Bitboard::EMPTY); }
    private:
        Bitboard bb;
};

constexpr std::array<Bitboard, num_of<Square>> BB_KNIGHT_ATTACKS = []() {
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

constexpr std::array<Bitboard, num_of<Square>> BB_KING_ATTACKS = []() {
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

constexpr std::array<std::array<Bitboard, num_of<Square>>, num_of<Color>> BB_PAWN_ATTACKS = []() {
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

template<Piece slider_piece, bool first = false, bool last = false>
constexpr Bitboard bb_slider_attack_mask(Square sq) {
    Bitboard attack = Bitboard::EMPTY;
    if constexpr (slider_piece == Piece::ROOK || slider_piece == Piece::QUEEN) {
        attack |= (
            ray<first, last>(sq, Direction::NORTH) | ray<first, last>(sq, Direction::EAST) |
            ray<first, last>(sq, Direction::SOUTH) | ray<first, last>(sq, Direction::WEST)
        );
    }
    if constexpr (slider_piece == Piece::BISHOP || slider_piece == Piece::QUEEN) {
        attack |= (
            ray<first, last>(sq, Direction::NORTHEAST) | ray<first, last>(sq, Direction::SOUTHEAST) |
            ray<first, last>(sq, Direction::NORTHWEST) | ray<first, last>(sq, Direction::SOUTHWEST)
        );
    }
    return attack;
}


template<Piece slider_piece>
constexpr std::array<Bitboard, num_of<Square>> BB_ATTACK_MASK = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square s : iter<Square>) {
        table[idx(s)] = bb_slider_attack_mask<slider_piece>(s);
    }
    return table;
}();


struct MagicEntry {
    Bitboard bb_blockers;
    Bitboard bb_moves;
};

template<Piece castle_side>
constexpr auto BB_CASTLE_PATHS = []() -> std::array<Bitboard, num_of<Color>> {
    static_assert(castle_side == Piece::KING || castle_side == Piece::QUEEN);
    if constexpr (castle_side == Piece::KING) {
        return {
            bb_square(Square::F1) | bb_square(Square::G1),
            bb_square(Square::F8) | bb_square(Square::G8)
        };
    } else {
        return {
            bb_square(Square::D1) | bb_square(Square::C1) | bb_square(Square::B1),
            bb_square(Square::D8) | bb_square(Square::C8) | bb_square(Square::B8)
        };
    }
}();

#ifdef PEXT_SUPPORT
inline size_t pext(Bitboard bb_occupied, Bitboard bb_attack_mask) {
    return _pext_u64(idx(bb_occupied), idx(bb_attack_mask));
}
#endif

class SliderAttacks {
    public:
        SliderAttacks() {};
        SliderAttacks(Square from, Piece slider_type);

        inline Bitboard bb_attacks(Bitboard bb_occupied) const {
            return bb_attacks_table[gen_hash(bb_occupied)];
        }

    private:
        inline size_t gen_hash(Bitboard bb_occupied) const {
            #ifndef PEXT_SUPPORT
                return idx(bb_occupied & bb_attack_mask) * magic >> shift;
            #else
                return pext(bb_occupied, bb_attack_mask);
            #endif
        }

        Bitboard bb_attack_mask;
        std::vector<Bitboard> bb_attacks_table;
        #ifndef PEXT_SUPPORT
        uint64_t magic;
        uint64_t shift;
        #endif
};

extern SliderAttacks ROOK_ATTACKS[num_of<Square>];
extern SliderAttacks BISHOP_ATTACKS[num_of<Square>];

template<Piece piece_type> requires is_knight_or_king<piece_type>
inline Bitboard bb_attacks(Square from) {
    if constexpr (piece_type == Piece::KNIGHT)
        return BB_KNIGHT_ATTACKS[idx(from)];
    if constexpr (piece_type == Piece::KING)
        return BB_KING_ATTACKS[idx(from)];

    return Bitboard::EMPTY;
}

template<Piece piece_type> requires is_major_piece<piece_type>
inline Bitboard bb_attacks(Square from, Bitboard bb_occupied) {
    if constexpr (piece_type == Piece::BISHOP)
        return BISHOP_ATTACKS[idx(from)].bb_attacks(bb_occupied);
    if constexpr (piece_type == Piece::ROOK)
        return ROOK_ATTACKS[idx(from)].bb_attacks(bb_occupied);
    if constexpr (piece_type == Piece::QUEEN)
        return (
            BISHOP_ATTACKS[idx(from)].bb_attacks(bb_occupied)
            | ROOK_ATTACKS[idx(from)].bb_attacks(bb_occupied)
        );
    if constexpr (piece_type == Piece::KNIGHT)
        return bb_attacks<Piece::KNIGHT>(from);
    if constexpr (piece_type == Piece::KING)
        return bb_attacks<Piece::KING>(from);

    return Bitboard::EMPTY;
}

template<Color color, Piece piece_type> requires (piece_type == Piece::PAWN)
inline Bitboard bb_attacks(Square from) {
    return BB_PAWN_ATTACKS[idx(color)][idx(from)];
}

template<Piece piece_type> requires (piece_type == Piece::PAWN)
inline Bitboard bb_attacks(Color color, Square from) {
    return BB_PAWN_ATTACKS[idx(color)][idx(from)];
}


} // namespace bears_chess
