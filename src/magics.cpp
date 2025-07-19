#include "magics.hpp"
#ifndef PEXT_SUPPORT
#include "magic_defs.hpp"
#endif

namespace bears_chess {

SliderAttacks ROOK_ATTACKS[num_of<Square>];
SliderAttacks BISHOP_ATTACKS[num_of<Square>];

static Bitboard assign_bits(Bitboard bb, uint64_t bits) {
    Bitboard bb_pattern = Bitboard::EMPTY;
    for (Square sq : BBSquareScan(bb)) {
        bb_pattern |= static_cast<Bitboard>((bits & 1) << idx(sq));
        bits >>= 1;
    }
    return bb_pattern;
}

template<Piece slider_type> requires is_bishop_or_rook<slider_type>
static Bitboard generate_slider_attacks(Bitboard bb_from, Bitboard bb_blockers) {
    Bitboard bb_attacks = Bitboard::EMPTY;
    for (Direction dir : SLIDER_DIRECTIONS<slider_type>) {
        Bitboard bb_bit = bb_shift<true>(bb_from, dir);
        while (nonzero(bb_bit)) {
            bb_attacks |= bb_bit;
            bb_bit = bb_shift<true>(bb_bit & ~bb_blockers, dir);
        }
    }
    return bb_attacks;
}

static Bitboard generate_slider_attacks(Bitboard bb_from, Bitboard bb_blocks, Piece slider_type) {
    switch (slider_type) {
        case Piece::ROOK:
            return generate_slider_attacks<Piece::ROOK>(bb_from, bb_blocks);
        case Piece::BISHOP:
            return generate_slider_attacks<Piece::BISHOP>(bb_from, bb_blocks);
        default:
            throw std::invalid_argument("Invalid piece type");
    }
}

static Bitboard bb_slider_attack(Square from, Piece slider_type) {
    switch (slider_type) {
        case Piece::ROOK:
            return bb_slider_attack<Piece::ROOK, false, false>(from);
        case Piece::BISHOP:
            return bb_slider_attack<Piece::BISHOP, false, false>(from);
        default:
            throw std::invalid_argument("Invalid piece type");
    }
}

#ifndef PEXT_SUPPORT
static uint64_t get_magic(Square from, Piece slider_type) {
    switch (slider_type) {
        case Piece::ROOK:
            return MAGICS<Piece::ROOK>[idx(from)];
        case Piece::BISHOP:
            return MAGICS<Piece::BISHOP>[idx(from)];
        default:
            throw std::invalid_argument("Invalid piece type");
    }
}
#endif

SliderAttacks::SliderAttacks(Square from, Piece slider_type) {
    bb_attack_mask = bb_slider_attack(from, slider_type);
    size_t table_size = (1ULL << popcount(bb_attack_mask));
    #ifndef PEXT_SUPPORT
    magic = get_magic(from, slider_type);
    shift = 64 - popcount(bb_attack_mask);
    #endif
    
    bb_attacks_table.resize(table_size);
    for (uint64_t bits = 0; bits < table_size; ++bits) {
        Bitboard bb_blockers = assign_bits(bb_attack_mask, bits);
        bb_attacks_table[gen_hash(bb_blockers)] = generate_slider_attacks(bb_square(from), bb_blockers, slider_type);
    }
}

template<Piece piece_type> requires is_bishop_or_rook<piece_type>
void init_attack_table(SliderAttacks attacks[num_of<Square>]) {
    for (Square sq : iter<Square>) {
        attacks[idx(sq)] = SliderAttacks(sq, piece_type);
    }
}

void init() {
    init_attack_table<Piece::ROOK>(ROOK_ATTACKS);
    init_attack_table<Piece::BISHOP>(BISHOP_ATTACKS);
}

} // namespace bears_chess