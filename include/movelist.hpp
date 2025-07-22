#pragma once
#include "types.hpp"

namespace bears_chess {

constexpr int MAX_MOVES = 218;

class MoveList {
public:
    MoveList() : cur(moves) {};

    inline void emplace_back(Move move) {
        *(cur++) = move;
    }

    inline void emplace_back(Square from, Square to, MoveType move_type) {
        *(cur++) = {from, to, move_type};
    }
    
    inline bool empty() const { return cur == moves; }
    inline size_t size() const { return cur - moves; }

    const Move* cbegin() const { return moves; }
    const Move* cend() const { return cur; }

    Move* begin() { return moves; }
    Move* end() { return cur; }
private:
    Move* cur;
    Move moves[MAX_MOVES];
};

} // namespace bears_chess