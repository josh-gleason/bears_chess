#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/board.hpp"

#include <array>
#include <utility>
#include <span>

namespace bears_chess {

struct PolicyMove {
    Square from;
    Square to;
    Piece promotion;
};

namespace detail {

constexpr int file_dist(Square from, Square to) {
    int d = idx(file_of(from)) - idx(file_of(to));
    return d < 0 ? -d : d;
}

constexpr int rank_dist(Square from, Square to) {
    int d = idx(rank_of(from)) - idx(rank_of(to));
    return d < 0 ? -d : d;
}

constexpr bool slider_reachable(Square from, Square to) {
    const int df = file_dist(from, to);
    const int dr = rank_dist(from, to);
    return from != to && (df == 0 || dr == 0 || df == dr);
}

constexpr bool knight_reachable(Square from, Square to) {
    const int df = file_dist(from, to);
    const int dr = rank_dist(from, to);
    return (df == 2 && dr == 1) || (df == 1 && dr == 2);
}

constexpr bool promotion(Square from, Square to) {
    return rank_of(from) == Rank::_7 && rank_of(to) == Rank::_8 && file_dist(from, to) <= 1;
}

constexpr std::array<Piece, 4> PROMOTION_PIECES = {
    Piece::KNIGHT, Piece::BISHOP, Piece::ROOK, Piece::QUEEN
};

constexpr size_t count_policy_moves() {
    size_t total = 0;
    for (Square from : iter<Square>) {
        for (Square to : iter<Square>) {
            if (slider_reachable(from, to) || knight_reachable(from, to)) {
                total++;
            }
            if (promotion(from, to)) {
                total += PROMOTION_PIECES.size();
            }
        }
    }
    return total;
}

constexpr size_t promote_index(Piece piece) {
    switch(piece) {
        case Piece::NONE:
            return 0;
        case Piece::KNIGHT:
        case Piece::BISHOP:
        case Piece::ROOK:
        case Piece::QUEEN:
            return 1 + idx(piece);
        default:
            std::unreachable();
    }
}

constexpr size_t policy_key(const PolicyMove& pm) {
    return idx(pm.from) * num_of<Square> * 5 + idx(pm.to) * 5 + promote_index(pm.promotion);
}

} // namespace detail

constexpr size_t POLICY_SIZE = detail::count_policy_moves();
static_assert(POLICY_SIZE == 1880);

// index -> policy_move
constexpr std::array<PolicyMove, POLICY_SIZE> POLICY_MOVES = []() {
    std::array<PolicyMove, POLICY_SIZE> table{};
    size_t i = 0;
    for (Square from: iter<Square>) {
        for (Square to: iter<Square>) {
            if (detail::slider_reachable(from, to) || detail::knight_reachable(from, to)) {
                table[i++] = PolicyMove{from, to, Piece::NONE};
            }
            if (detail::promotion(from, to)) {
                for (Piece promotion_piece : detail::PROMOTION_PIECES) {
                    table[i++] = PolicyMove{from, to, promotion_piece};
                }
            }
        }
    }
    return table;
}();

constexpr int16_t NO_POLICY_INDEX = -1;

class PolicyIndexTable {
public:
    constexpr PolicyIndexTable() {
        table.fill(NO_POLICY_INDEX);
        for (size_t i = 0; i < POLICY_SIZE; ++i) {
            table[detail::policy_key(POLICY_MOVES[i])] = static_cast<int16_t>(i);
        }
    }

    constexpr int16_t operator[](const PolicyMove& pm) const {
        return table[detail::policy_key(pm)];
    }
private:
    std::array<int16_t, num_of<Square> * num_of<Square> * 5> table{};
};

// policy_move -> index
constexpr PolicyIndexTable POLICY_INDICES{};

constexpr Square canonical_square(Square square, Color side_to_move) {
    return side_to_move == Color::WHITE ? square : flip_square(square);
}

constexpr PolicyMove policy_move(const Move& move, Color side_to_move) {
    return {
        canonical_square(move.from, side_to_move),
        canonical_square(move.to, side_to_move),
        promote_to_or_none(move.move_type)
    };
}

constexpr size_t policy_index(const Move& move, Color side_to_move) {
    int16_t index = POLICY_INDICES[policy_move(move, side_to_move)];
    assert(index != NO_POLICY_INDEX);
    return static_cast<size_t>(index);
}

constexpr size_t INPUT_PLANES = 18;
constexpr size_t INPUT_SIZE = INPUT_PLANES * num_of<Square>;

class InputPlanes {
public:
    enum Plane : size_t {
        OWN_PIECES = 0,
        OPPONENT_PIECES = 6,
        OWN_CASTLE_KING = 12,
        OWN_CASTLE_QUEEN = 13,
        OPPONENT_CASTLE_KING = 14,
        OPPONENT_CASTLE_QUEEN = 15,
        EN_PASSANT = 16,
        HALFMOVE_CLOCK = 17,
        COUNT = 18
    };

    static constexpr size_t SIZE = COUNT * num_of<Square>;

    float& at(size_t plane, Square square) {
        return storage[plane * num_of<Square> + idx(square)];
    }

    float at(size_t plane, Square square) const {
        return storage[plane * num_of<Square> + idx(square)];
    }

    std::span<float, num_of<Square>> plane(size_t plane) {
        return std::span<float, num_of<Square>>(storage.data() + plane * num_of<Square>, num_of<Square>);
    }

    std::span<const float, num_of<Square>> plane(size_t plane) const {
        return std::span<const float, num_of<Square>>(storage.data() + plane * num_of<Square>, num_of<Square>);
    }

    std::span<const float> view() const {
        return storage;
    }

private:
    std::array<float, SIZE> storage{};
};

InputPlanes encode_board(const Board& board);

} // namespace bears_chess
