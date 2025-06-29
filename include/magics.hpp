#pragma once

#include <vector>
#include <unordered_map>
#include <iomanip>
#include <span>
#include <ranges>

#include "types.hpp"
#include "bitboard.hpp"
#include "magic_defs.hpp"

namespace bears_chess {

template<Piece slider_piece>
constexpr std::array<Direction, 4> SLIDER_DIRECTIONS = []() {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    if constexpr (slider_piece == Piece::ROOK) {
        return CARDINAL_DIRECTIONS;
    } else {
        return ORDINAL_DIRECTIONS;
    }
}();

constexpr size_t log2(size_t n) {
    return (n > 1) ? 1 + log2(n / 2) : 0;
}

constexpr size_t magic_hash(Bitboard bb_blockers, uint64_t magic, size_t target_size) {
    return (static_cast<uint64_t>(bb_blockers) * magic) >> (64 - log2(target_size));
}

constexpr Bitboard assign_bits(Bitboard bb, uint64_t bits) noexcept {
    Bitboard bb_pattern = Bitboard::EMPTY;
    for (Square sq : BBSquareScan(bb)) {
        bb_pattern |= static_cast<Bitboard>((bits & 1) << idx(sq));
        bits >>= 1;
    }
    return bb_pattern;
}

template<size_t D>
constexpr Bitboard generate_bb_slider_moves(Bitboard bb_loc, Bitboard bb_blockers, const std::array<Direction, D>& directions) noexcept {
    Bitboard bb_moves = Bitboard::EMPTY;
    for (Direction dir : directions) {
        Bitboard bb_bit = bb_shift<true>(bb_loc, dir);
        while (nonzero(bb_bit)) {
            bb_moves |= bb_bit;
            if (nonzero(bb_blockers & bb_bit))
                break;
            bb_bit = bb_shift<true>(bb_bit, dir);
        }
    }
    return bb_moves;
}

template<Piece slider_piece>
constexpr auto attack_view(Square sq) {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    Bitboard bb_attack = BB_ATTACK_MASK<slider_piece>[idx(sq)];
    Bitboard bb_loc = bb_square(sq);
    return (
        std::views::iota(0ULL, 1ULL << popcount(bb_attack)) |
        std::views::transform([bb_attack, bb_loc](size_t bits) {
            Bitboard bb_blockers = assign_bits(bb_attack, bits);
            Bitboard bb_moves = generate_bb_slider_moves(bb_loc, bb_blockers, SLIDER_DIRECTIONS<slider_piece>);
            return MagicEntry{bb_blockers, bb_moves};
        })
    );
}

template<Piece slider_piece>
constexpr std::array<size_t, num_of<Square>> magic_table_size = []() {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    std::array<size_t, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = 1ULL << popcount(BB_ATTACK_MASK<slider_piece>[idx(sq)]);
    }
    return table;
}();

template<Piece slider_piece>
constexpr std::vector<Bitboard> generate_magic_attack_table(Square square) {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    size_t target_size = magic_table_size<slider_piece>[idx(square)];
    std::vector<Bitboard> table(target_size, Bitboard::EMPTY);
    uint64_t magic = MAGICS<slider_piece>[idx(square)];
    for (MagicEntry attack_set : attack_view<slider_piece>(square)) {
        uint64_t hash = magic_hash(attack_set.bb_blockers, magic, target_size);
        table[hash] = attack_set.bb_moves;
    }
    return table;
}

// compute these at run time for now to avoid excessive compile times
template<Piece slider_piece>
constexpr const std::array<std::vector<Bitboard>, num_of<Square>>& get_magic_tables() noexcept {
    // static_assert(
    //     slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
    //     "magics available only for slider_piece of ROOK or BISHOP"
    // );
    // static std::array<std::vector<Bitboard>, num_of<Square>> table = []() {
    //     std::array<std::vector<Bitboard>, num_of<Square>> table{};
    //     for (Square sq : iter<Square>) {
    //         table[idx(sq)] = magic_table<slider_piece>(sq);
    //     }
    //     return table;
    // }();
    // return table;
    static std::array<std::vector<Bitboard>, num_of<Square>> table{};
    static bool initialized = false;
    if (!initialized) {
        for (Square sq : iter<Square>)
            table[idx(sq)] = generate_magic_attack_table<slider_piece>(sq);
        initialized = true;
    }
    return table;
}

template<Piece slider_piece>
inline Bitboard get_bb_slider_moves(Square sq, Bitboard bb_blockers) noexcept {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    size_t target_size = magic_table_size<slider_piece>[idx(sq)];
    uint64_t magic = MAGICS<slider_piece>[idx(sq)];
    size_t hash = magic_hash(bb_blockers, magic, target_size);
    return get_magic_tables<slider_piece>()[idx(sq)][hash];
}

// template<Piece slider_piece>
// inline Bitboard get_bb_slider_moves(Square sq, Bitboard bb_blockers) noexcept {
//     static_assert(
//         slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
//         "magics available only for slider_piece of ROOK or BISHOP"
//     );
//     return generate_bb_slider_moves(bb_square(sq), bb_blockers, SLIDER_DIRECTIONS<slider_piece>);
// }

}    // namespace bears_chess
