#pragma once

#include <array>
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

constexpr size_t magic_hash(Bitboard bb_blockers, uint64_t magic, int shift) {
    return (static_cast<uint64_t>(bb_blockers) * magic) >> shift;
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
constexpr std::array<int, num_of<Square>> magic_table_shift = []() {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    std::array<int, num_of<Square>> table{};
    for (Square sq : iter<Square>) {
        table[idx(sq)] = 1 + __builtin_clzll(magic_table_size<slider_piece>[idx(sq)]);
    }
    return table;
}();

template<Piece slider_piece>
constexpr std::array<size_t, num_of<Square> + 1> magic_table_offset = []() {
    static_assert(
        slider_piece == Piece::ROOK || slider_piece == Piece::BISHOP,
        "magics available only for slider_piece of ROOK or BISHOP"
    );
    std::array<size_t, num_of<Square> + 1> table{};
    size_t offset = 0;
    for (Square sq : iter<Square>) {
        table[idx(sq)] = offset;
        offset += magic_table_size<slider_piece>[idx(sq)];
    }
    table[idx(Square::NONE)] = offset;
    return table;
}();

template<Piece slider_piece>
constexpr size_t magic_table_total_size = magic_table_offset<slider_piece>[num_of<Square>];

template<Piece slider_piece>
extern const std::array<Bitboard, magic_table_total_size<slider_piece>> MAGIC_ATTACK_TABLE;

template<Piece slider_piece>
inline Bitboard magic_lookup(Square from, Bitboard bb_occupied) noexcept {
    Bitboard bb_attack_mask = BB_ATTACK_MASK<slider_piece>[idx(from)];
    Bitboard bb_blockers = bb_attack_mask & bb_occupied;
    uint64_t magic = MAGICS<slider_piece>[idx(from)];
    int shift = magic_table_shift<slider_piece>[idx(from)];
    size_t offset = magic_table_offset<slider_piece>[idx(from)];
    size_t hash = magic_hash(bb_blockers, magic, shift);
    return MAGIC_ATTACK_TABLE<slider_piece>[offset + hash];
}

}    // namespace bears_chess
