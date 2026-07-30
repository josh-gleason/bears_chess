#pragma once
#include "types.hpp"
#include "bitboard.hpp"

namespace bears_chess {

constexpr int MAX_MOVES = 218;

class MoveList {
public:
    typedef std::array<Move, MAX_MOVES> ListType;
    typedef ListType::iterator iterator;
    typedef ListType::const_iterator const_iterator;

    MoveList() {
        cur = moves.begin();
    }

    MoveList(const MoveList& rhs) : moves(rhs.moves), cur(moves.begin() + rhs.size()) {}

    MoveList& operator=(const MoveList& rhs) {
        moves = rhs.moves;
        cur = moves.begin() + rhs.size();
        return *this;
    }

    inline void emplace_back(Move move) {
        *(cur++) = move;
    }

    inline void emplace_back(Square from, Square to, MoveType move_type) {
        *(cur++) = {from, to, move_type};
    }
    
    inline bool empty() const { return cur == moves.begin(); }
    inline size_t size() const { return cur - moves.begin(); }

    const_iterator cbegin() const { return moves.cbegin(); }
    const_iterator cend() const { return cur; }

    const_iterator begin() const { return moves.cbegin(); }
    const_iterator end() const { return cur; }

    iterator begin() { return moves.begin(); }
    iterator end() { return cur; }

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

private:
    ListType moves;
    iterator cur;
};

} // namespace bears_chess