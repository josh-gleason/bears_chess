#pragma once
#include "types.hpp"

namespace bears_chess {

constexpr int MAX_MOVES = 218;

class MoveList {
public:
    typedef std::array<Move, MAX_MOVES> ListType;
    typedef ListType::iterator iterator;
    typedef ListType::const_iterator const_iterator;

#pragma gcc diagnostic push
#pragma clang diagnostic push
#pragma gcc diagnostic ignored "-Wuninitialized"
#pragma clang diagnostic ignored "-Wuninitialized"
    MoveList() : cur(moves.begin()) {}
#pragma gcc diagnostic pop
#pragma clang diagnostic pop

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

    iterator begin() { return moves.begin(); }
    iterator end() { return cur; }
private:
    iterator cur;
    ListType moves;
};

} // namespace bears_chess