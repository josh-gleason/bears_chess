#include "bitboard.hpp"
#include "board_utils.hpp"

#include <span>

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

constexpr std::array<Bitboard, num_of<Square>> BB_ROOK_ATTACK_PREMASK = []() {
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

constexpr std::array<Bitboard, num_of<Square>> BB_ROOK_ATTACK_POSTMASK = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square s : iter<Square>) {
        table[idx(s)] = (
            ray<false, true>(s, Direction::NORTH) |
            ray<false, true>(s, Direction::EAST) |
            ray<false, true>(s, Direction::SOUTH) |
            ray<false, true>(s, Direction::WEST)
        );
    }
    return table;
}();

constexpr std::array<Bitboard, num_of<Square>> BB_BISHOP_ATTACK_PREMASK = []() {
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

constexpr std::array<Bitboard, num_of<Square>> BB_BISHOP_ATTACK_POSTMASK = []() {
    std::array<Bitboard, num_of<Square>> table{};
    for (Square s : iter<Square>) {
        table[idx(s)] = (
            ray<false, true>(s, Direction::NORTHEAST) |
            ray<false, true>(s, Direction::SOUTHEAST) |
            ray<false, true>(s, Direction::SOUTHWEST) |
            ray<false, true>(s, Direction::NORTHWEST)
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

class BBBlockersIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Bitboard;
        using difference_type = std::ptrdiff_t;
        using pointer = const Bitboard*;
        using reference = const Bitboard&;

        constexpr BBBlockersIterator(Bitboard _bb_attack, bool is_end = false) noexcept :
            bb_attack(_bb_attack),
            count(is_end ? (1 << popcount(_bb_attack)) : 0)
        {}

        constexpr bool operator!=(const BBBlockersIterator& other) const noexcept {
            return count != other.count;
        }

        constexpr Bitboard operator*() const noexcept {
            return assign_bits(bb_attack, count);
        }

        constexpr BBBlockersIterator& operator++() noexcept {
            count++;
            return *this;
        }

    private:
        Bitboard bb_attack;
        uint64_t count;
};

class BBBlockersRange {
    public:
        constexpr BBBlockersRange(Bitboard _bb_attack) noexcept :
            bb_attack(_bb_attack)
        {}

        constexpr BBBlockersIterator begin() const noexcept {
            return BBBlockersIterator(bb_attack);
        }

        constexpr BBBlockersIterator end() const noexcept {
            return BBBlockersIterator(bb_attack, true);
        }
    private:
        Bitboard bb_attack;
};

struct AttackSet {
    Bitboard bb_blockers;
    Bitboard bb_moves;
};

template<std::array<Bitboard, num_of<Square>> attack_masks, std::array<Direction, 4> directions>
class BBSlideAttackSetIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = AttackSet;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        constexpr BBSlideAttackSetIterator(Square sq, bool is_end = false) noexcept :
            bb_blockers_it(attack_masks[idx(sq)], is_end),
            bb_attack(attack_masks[idx(sq)]),
            bb_location(bb_square(sq))
        {}

        constexpr bool operator!=(const BBSlideAttackSetIterator& other) const noexcept {
            return bb_blockers_it != other.bb_blockers_it;
        }

        constexpr AttackSet operator*() const noexcept {
            Bitboard bb_blockers = *bb_blockers_it;
            Bitboard bb_moves = Bitboard::EMPTY;
            for (Direction dir : directions) {
                Bitboard bb_bit = bb_shift<true>(bb_location, dir);
                while (nonzero(bb_bit)) {
                    bb_moves |= bb_bit;
                    if (nonzero(bb_bit & bb_blockers))
                        break;
                    bb_bit = bb_shift<true>(bb_bit, dir);
                }
            }
            return AttackSet{bb_blockers, bb_moves};
        }

        constexpr BBSlideAttackSetIterator& operator++() noexcept {
            ++bb_blockers_it;
            return *this;
        }

    private:
        BBBlockersIterator bb_blockers_it;
        Bitboard bb_attack;
        Bitboard bb_location;
};

template<std::array<Bitboard, num_of<Square>> attack_masks, std::array<Direction, 4> directions>
class BBSlideAttackSetRange {
    public:
        using iterator = BBSlideAttackSetIterator<attack_masks, directions>;
        using const_iterator = iterator;
        using value_type = iterator::value_type;
        using reference = value_type&;
        using const_reference = const value_type&;

        constexpr BBSlideAttackSetRange(Square _sq) noexcept :
            sq(_sq)
        {}

        constexpr iterator begin() const noexcept {
            return iterator(sq);
        }

        constexpr iterator end() const noexcept {
            return iterator(sq, true);
        }
    private:
        Square sq;
};

using BBRookAttackSetRange = BBSlideAttackSetRange<BB_ROOK_ATTACK_PREMASK, CARDINAL_DIRECTIONS>;
using BBBishopAttackSetRange = BBSlideAttackSetRange<BB_BISHOP_ATTACK_PREMASK, ORDINAL_DIRECTIONS>;

}   // namespace bears_chess

#include <iostream>

int main() {
    using namespace bears_chess;

    for (Square sq : iter<Square>) {
        BBRookAttackSetRange rng(sq);
        std::vector<AttackSet> attack_set(rng.begin(), rng.end());

        std::cout << sq << " " << attack_set.size() << std::endl;
        
        // AttackSet attack_set = *iter;
        // std::cout << sq << std::endl;
        // std::cout << show_highlights(attack_set.bb_blockers, attack_set.bb_moves);
    }

    return 0;
}