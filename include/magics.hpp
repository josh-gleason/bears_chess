#pragma once

#include <array>
#include <ranges>
#include <memory>
#include "types.hpp"
#include "bitboard.hpp"
#include "magic_defs.hpp"

namespace bears_chess {

void init();

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
                turn idx(bb_occupied & bb_attack_mask) * magic >> shift;
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

template<Piece piece_type> requires is_major_piece<piece_type>
inline Bitboard bb_attacks(Square from, Bitboard bb_occupied) {
    if constexpr (piece_type == Piece::BISHOP)
        return BISHOP_ATTACKS[idx(from)].bb_attacks(bb_occupied);
    if constexpr (piece_type == Piece::ROOK)
        return ROOK_ATTACKS[idx(from)].bb_attacks(bb_occupied);
    if constexpr (piece_type == Piece::QUEEN)
        return (
            ROOK_ATTACKS[idx(from)].bb_attacks(bb_occupied)
            | ROOK_ATTACKS[idx(from)].bb_attacks(bb_occupied)
        );
    if constexpr (piece_type == Piece::KNIGHT)
        return BB_KNIGHT_MOVES[idx(from)];
    if constexpr (piece_type == Piece::KING)
        return BB_KING_MOVES[idx(from)];

    return Bitboard::EMPTY;
}

template<Piece piece_type>
inline Bitboard magic_lookup(Square from, Bitboard bb_occupied) {
    return bb_attacks<piece_type>(from, bb_occupied);
}

} // namespace bears_chess