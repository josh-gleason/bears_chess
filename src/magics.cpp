#include "magics.hpp"
#include <ranges>

namespace bears_chess {

template<Piece slider_piece>
constexpr std::array<Bitboard, magic_table_total_size<slider_piece>> build_magic_attack_table() {
    std::array<Bitboard, magic_table_total_size<slider_piece>> table{};
    for (Square sq : iter<Square>) {
        size_t offset = magic_table_offset<slider_piece>[idx(sq)];
        int shift = magic_table_shift<slider_piece>[idx(sq)];
        uint64_t magic = MAGICS<slider_piece>[idx(sq)];
        for (MagicEntry entry : attack_view<slider_piece>(sq)) {
            size_t hash = magic_hash(entry.bb_blockers, magic, shift);
            table[offset + hash] = entry.bb_moves;
        }
    }
    return table;
}

template<>
const auto MAGIC_ATTACK_TABLE<Piece::ROOK> = build_magic_attack_table<Piece::ROOK>();

template<>
const auto MAGIC_ATTACK_TABLE<Piece::BISHOP> = build_magic_attack_table<Piece::BISHOP>();

}   // namespace bears_chess
