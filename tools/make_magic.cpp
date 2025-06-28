#include "bitboard.hpp"
#include "board_utils.hpp"

#include <span>
#include <ranges>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include <random>
#include <bitset>
#include <vector>

namespace bears_chess {

enum class Direction : int8_t {
    NORTH = 8,
    EAST = 1,
    SOUTH = -8,
    WEST = -1,
    NORTHEAST = NORTH + EAST,
    SOUTHEAST = SOUTH + EAST,
    SOUTHWEST = SOUTH + WEST,
    NORTHWEST = NORTH + WEST
};

constexpr std::array<Direction, 4> CARDINAL_DIRECTIONS = {
    Direction::NORTH, Direction::EAST, Direction::SOUTH, Direction::WEST
};

constexpr std::array<Direction, 4> ORDINAL_DIRECTIONS = {
    Direction::NORTHEAST, Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::NORTHWEST
};

constexpr int hash(Direction e) noexcept {
    return (static_cast<int8_t>(e) + 9) % 11;
}

template<typename T, size_t N>
constexpr bool all_unique(const std::array<T, N>& arr) {
    for (size_t i = 0; i < N; ++i)
        for (size_t j = i + 1; j < N; ++j)
            if (arr[i] == arr[j])
                return false;
    return true;
}

constexpr std::array<Bitboard, 11> BB_DIRECTION_PREMASK = []() {
    static_assert([]{
        constexpr std::array<int, 8> hashes = {
            hash(Direction::NORTH), hash(Direction::EAST),
            hash(Direction::SOUTH), hash(Direction::WEST),
            hash(Direction::NORTHEAST), hash(Direction::NORTHWEST),
            hash(Direction::SOUTHEAST), hash(Direction::SOUTHWEST)
        };
        if (!all_unique(hashes)) return false;
        for (int h : hashes)
            if (h < 0 || h > 10) return false;
        return true;
    }(), "Direction hash is invalid or out of range");

    std::array<Bitboard, 11> table{};
    for (int i = 0; i < 11; ++i) {
        table[i] = Bitboard::FULL;    
    }
    table[hash(Direction::EAST)] = ~bb_file(File::H);
    table[hash(Direction::NORTHEAST)] = ~bb_file(File::H);
    table[hash(Direction::SOUTHEAST)] = ~bb_file(File::H);
    table[hash(Direction::WEST)] = ~bb_file(File::A);
    table[hash(Direction::SOUTHWEST)] = ~bb_file(File::A);
    table[hash(Direction::NORTHWEST)] = ~bb_file(File::A);
    return table;
}();

constexpr Bitboard bb_direction_premask(Direction dir) noexcept {
    return BB_DIRECTION_PREMASK[hash(dir)];
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

constexpr std::array<Bitboard, num_of<Square>> BB_ROOK_ATTACK_MASK = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square s : iter<Square>) {
        table[idx(s)] = (
            ray(s, Direction::NORTH) |
            ray(s, Direction::EAST) |
            ray(s, Direction::SOUTH) |
            ray(s, Direction::WEST)
        );
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_BISHOP_ATTACK_MASK = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square s : iter<Square>) {
        table[idx(s)] = (
            ray(s, Direction::NORTHEAST) |
            ray(s, Direction::SOUTHEAST) |
            ray(s, Direction::SOUTHWEST) |
            ray(s, Direction::NORTHWEST)
        );
    }
    return table;
}();

constexpr Bitboard assign_bits(Bitboard bb, uint64_t bits) noexcept {
    Bitboard bb_pattern = Bitboard::EMPTY;
    for (Square sq : bb_square_scan(bb)) {
        bb_pattern |= static_cast<Bitboard>((bits & 1) << idx(sq));
        bits >>= 1;
    }
    return bb_pattern;
}

struct AttackSet {
    Bitboard bb_blockers;
    Bitboard bb_moves;
};

template<std::array<Direction, 4> directions>
constexpr AttackSet make_attack_set(Bitboard bb_attack, Bitboard bb_loc, uint64_t bits) noexcept {
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

inline auto rook_attack_view(Square sq) {
    Bitboard bb_attack = BB_ROOK_ATTACK_MASK[idx(sq)];
    Bitboard bb_loc = bb_square(sq);
    return (
        std::views::iota(0ULL, 1ULL << popcount(bb_attack)) |
        std::views::transform(
            [bb_attack, bb_loc](size_t bits) {
                return make_attack_set<CARDINAL_DIRECTIONS>(bb_attack, bb_loc, bits);
            }
        )
    );
}

inline auto bishop_attack_view(Square sq) {
    Bitboard bb_attack = BB_BISHOP_ATTACK_MASK[idx(sq)];
    Bitboard bb_loc = bb_square(sq);
    return (
        std::views::iota(0ULL, 1ULL << popcount(bb_attack)) |
        std::views::transform(
            [bb_attack, bb_loc](size_t bits) {
                return make_attack_set<ORDINAL_DIRECTIONS>(bb_attack, bb_loc, bits);
            }
        )
    );
}

constexpr size_t log2(size_t n) {
    return (n > 1) ? 1 + log2(n / 2) : 0;
}

size_t is_valid_magic(uint64_t magic, const std::vector<AttackSet>& attack_sets, size_t size) {
    std::unordered_map<size_t, Bitboard> hash_to_moves;

    size_t max_hash = 0;
    for (const auto& attack_set : attack_sets) {
        size_t hash = ((static_cast<uint64_t>(attack_set.bb_blockers) * magic) >> (64 - log2(size))) % size;

        if (hash_to_moves.count(hash)) {
            // if the hash exists, ensure the moves are the same
            if (hash_to_moves[hash] != attack_set.bb_moves) {
                return 0;
            }
        } else {
            // otherwise, store the hash and the moves
            hash_to_moves[hash] = attack_set.bb_moves;
        }
        max_hash = std::max(hash, max_hash);
    }

    return max_hash;
}

uint64_t find_magic(const std::vector<AttackSet>& attack_sets, size_t size) {
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist;

    while (true) {
        uint64_t magic = dist(rng) & dist(rng) & dist(rng); // generate a random magic number
        size_t max_hash = is_valid_magic(magic, attack_sets, size);
        if (max_hash > 0) {
            return magic; // found a valid magic number
        }
    }
}

}   // namespace bears_chess


int main() {
    using namespace bears_chess;

    std::ofstream fout("../../include/magics.hpp");

    fout << "#pragma once\n"
         << "\n"
         << "// File automatically generated by " << __FILE__ << "\n"
         << "\n"
         << "#include \"types.hpp\"\n"
         << "\n"
         << "namespace bears_chess {\n"
         << "\n"
         << "struct MagicInfo {\n"
         << "    uint64_t magic;\n"
         << "    int shift;\n"
         << "};\n"
         << std::endl;
    fout << "constexpr std::array<MagicInfo, num_of<Square>> ROOK_MAGICS = {" << std::endl;
    for (Square sq : iter<Square>) {
        auto attack_set_view = rook_attack_view(sq);
        std::vector<AttackSet> attack_sets(attack_set_view.begin(), attack_set_view.end());

        size_t size = attack_sets.size();
        uint64_t magic = find_magic(attack_sets, size);

        fout << "    MagicInfo{0x" << std::hex << std::setw(16) << std::setfill('0') << magic
             << "ULL, " << std::dec << 64 - log2(size) << "}" << (sq != Square::H8 ? "," : "")
             << std::endl;
    }
    fout << "};" << std::endl;

    fout << "constexpr std::array<MagicInfo, num_of<Square>> BISHOP_MAGICS = {" << std::endl;
    for (Square sq : iter<Square>) {
        auto attack_set_view = bishop_attack_view(sq);
        std::vector<AttackSet> attack_sets(attack_set_view.begin(), attack_set_view.end());

        size_t size = attack_sets.size();
        uint64_t magic = find_magic(attack_sets, size);

        fout << "    MagicInfo{0x" << std::hex << std::setw(16) << std::setfill('0') << magic
             << "ULL, " << std::dec << 64 - log2(size) << "}" << (sq != Square::H8 ? "," : "")
             << std::endl;
    }
    fout << "};" << std::endl;

    fout << "\n"
         << "}    // namespace bears_chess\n"
         << std::endl;

    return 0;
}
