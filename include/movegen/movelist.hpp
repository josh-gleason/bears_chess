#pragma once

#include "types.hpp"
#include "bitboard.hpp"

#include <cstddef>
#include <algorithm>
#include <vector>
#include <cassert>

namespace bears_chess {

template <size_t MAX_LENGTH = 218>
class BasicMoveList: public StaticVector<Move, MAX_LENGTH> {
private:
    using TParent = StaticVector<Move, MAX_LENGTH>;
public:
    using ListType = TParent::ListType;
    using iterator = TParent::iterator;
    using const_iterator = TParent::const_iterator;

    using size_type = TParent::size_type;

    inline void append_bb(Square from, Bitboard bb_to, MoveType move_type) {
        for (Square to : BBSquareScan(bb_to)) {
            TParent::emplace_back(from, to, move_type);
        }
    }

    inline void append_bb(Bitboard bb_from, Square to, MoveType move_type) {
        for (Square from : BBSquareScan(bb_from)) {
            TParent::emplace_back(from, to, move_type);
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
        auto it = std::find(TParent::begin(), TParent::end(), front_move);
        if (it != TParent::end()) {
            std::swap(TParent::front(), *it);
        }
    }

    bool move_to(const Move& move, size_type to) {
        auto it = std::find(TParent::begin(), TParent::end(), move);
        if (it != TParent::end()) {
            std::swap(TParent::operator[](to), *it);
            return true;
        }
        return false;
    }
};

using MoveList = BasicMoveList<>;

} // namespace bears_chess
