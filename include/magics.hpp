#pragma once

#include <vector>
#include <unordered_map>
#include <iomanip>
#include <span>
#include <ranges>

#include "types.hpp"

namespace bears_chess {

constexpr size_t log2(size_t n) {
    return (n > 1) ? 1 + log2(n / 2) : 0;
}

template<uint64_t magic, size_t N>
constexpr size_t magic_hash(Bitboard bb_blockers) {
    return static_cast<uint64_t>(bb_blockers) * magic >> (64 - log2(N));
}

constexpr Bitboard assign_bits(Bitboard bb, uint64_t bits) noexcept {
    Bitboard bb_pattern = Bitboard::EMPTY;
    for (Square sq : bb_square_scan(bb)) {
        bb_pattern |= static_cast<Bitboard>((bits & 1) << idx(sq));
        bits >>= 1;
    }
    return bb_pattern;
}

template<size_t D>
constexpr AttackSet make_attack_set(Bitboard bb_attack, const std::array<Direction, D>& directions, Bitboard bb_loc, uint64_t bits) noexcept {
    Bitboard blockers = assign_bits(bb_attack, bits);
    Bitboard moves = Bitboard::EMPTY;
    for (Direction dir : directions) {
        Bitboard bb_bit = bb_shift<true>(bb_loc, dir);
        while (nonzero(bb_bit)) {
            moves |= bb_bit;
            if (nonzero(blockers & bb_bit))
                break;
            bb_bit = bb_shift<true>(bb_bit, dir);
        }
    }
    return { blockers, moves };
}

constexpr auto rook_attack_view(Square sq) {
    Bitboard bb_attack = BB_ROOK_ATTACK_MASK[idx(sq)];
    Bitboard bb_loc = bb_square(sq);
    return (
        std::views::iota(0ULL, 1ULL << popcount(bb_attack)) |
        std::views::transform(
            [bb_attack, bb_loc](size_t bits) {
                return make_attack_set(bb_attack, CARDINAL_DIRECTIONS, bb_loc, bits);
            }
        )
    );
}

constexpr auto bishop_attack_view(Square sq) {
    Bitboard bb_attack = BB_BISHOP_ATTACK_MASK[idx(sq)];
    Bitboard bb_loc = bb_square(sq);
    return (
        std::views::iota(0ULL, 1ULL << popcount(bb_attack)) |
        std::views::transform(
            [bb_attack, bb_loc](size_t bits) {
                return make_attack_set(bb_attack, CARDINAL_DIRECTIONS, bb_loc, bits);
            }
        )
    );
}


template<Piece slider_type, Square sq, uint64_t magic, size_t N>
constexpr std::array<Bitboard, N> magic_table() {
    std::array<Bitboard, N> table{};
    // iterate over rook or bishop blocker patterns for this square
    if constexpr (slider_type == Piece::ROOK) {
        for (AttackSet attack_set : rook_attack_view(sq)) {
            uint64_t hash = magic_hash<magic, N>(attack_set.bb_blockers);
            table[hash] = attack_set.bb_moves;
        }
    } else {
        for (AttackSet attack_set : bishop_attack_view(sq)) {
            uint64_t hash = magic_hash<magic, N>(attack_set.bb_blockers);
            table[hash] = attack_set.bb_moves;
        }
    }
    return table;
}

template<size_t N>
constexpr std::span<const Bitboard> span_of(const std::array<Bitboard, N>& arr) {
    return std::span<const Bitboard>(arr.data(), arr.size());
}

const std::array<std::span<const Bitboard>, 2> MAGIC_ROOK_TABLES = {
    magic_table<Piece::ROOK, Square::A1, 0x123541ULL, 4096>(),
    magic_table<Piece::ROOK, Square::A2, 0x1123412341ULL, 4096>()
};


}    // namespace bears_chess

