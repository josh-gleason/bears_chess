#pragma once

#include "types.hpp"
#include "bitboard.hpp"

#include <cstddef>
#include <algorithm>
#include <vector>
#include <array>
#include <cassert>
#include <span>

namespace bears_chess {

template <size_t MAX_LENGTH = 218>
class BasicMoveList {
public:
    using ListType = std::array<Move, MAX_LENGTH>;
    using iterator = ListType::iterator;
    using const_iterator = ListType::const_iterator;

    using size_type = size_t;

    static constexpr size_t max_length = MAX_LENGTH;

    BasicMoveList() : count(0), moves{} {}

    BasicMoveList(const BasicMoveList& rhs) : count{rhs.count} {
        std::copy(rhs.cbegin(), rhs.cend(), moves.begin());
    }

    BasicMoveList& operator=(const BasicMoveList& rhs) {
        if (this != &rhs) {
            count = rhs.count;
            std::copy(rhs.cbegin(), rhs.cend(), moves.begin());
        }
        return *this;
    }

    Move& operator[](size_type index) {
        assert(index < count);
        return moves[index];
    }

    const Move& operator[](size_type index) const {
        assert(index < count);
        return moves[index];
    }

    inline void emplace_back(Move move) {
        assert(count < MAX_LENGTH);
        moves[count++] = move;
    }

    inline void emplace_back(Square from, Square to, MoveType move_type) {
        assert(count < MAX_LENGTH);
        moves[count++] = {from, to, move_type};
    }
    
    inline bool empty() const { return count == 0; }
    inline size_t size() const { return count; }

    const_iterator cbegin() const { return moves.cbegin(); }
    const_iterator cend() const { return moves.cbegin() + count; }

    const_iterator begin() const { return moves.cbegin(); }
    const_iterator end() const { return moves.cbegin() + count; }

    iterator begin() { return moves.begin(); }
    iterator end() { return moves.begin() + count; }

    Move& front() {
        assert(count > 0);
        return *moves.begin();
    }

    const Move& front() const {
        assert(count > 0);
        return *moves.begin();
    }

    inline void clear() { count = 0; }

    inline void append(const BasicMoveList& moves) {
        for (const auto& move : moves) {
            emplace_back(move);
        }
    }

    inline void append_bb(Square from, Bitboard bb_to, MoveType move_type) {
        for (Square to : BBSquareScan(bb_to)) {
            emplace_back(from, to, move_type);
        }
    }

    inline void append_bb(Bitboard bb_from, Square to, MoveType move_type) {
        for (Square from : BBSquareScan(bb_from)) {
            emplace_back(from, to, move_type);
        }
    }

    BasicMoveList filter_moves(const std::vector<Move>& allowed_moves) const {
        BasicMoveList filtered_moves{};
        for (const Move& move : *this) {
            if (std::find(allowed_moves.cbegin(), allowed_moves.cend(), move) != allowed_moves.cend()) {
                filtered_moves.emplace_back(move);
            }
        }
        return filtered_moves;
    }

    void promote_to_front(const Move& front_move) {
        auto it = std::find(begin(), end(), front_move);
        if (it != end()) {
            std::swap(front(), *it);
        }
    }

    void move_to(const Move& move, size_type to) {
        assert(to < count);
        auto it = std::find(begin(), end(), move);
        if (it != end()) {
            std::swap(moves[to], *it);
        }
    }

    std::span<const Move> view() const {
        return std::span<const Move>(moves.cbegin(), count);
    }

private:
    size_type count;
    ListType moves;
};

using MoveList = BasicMoveList<>;

} // namespace bears_chess
