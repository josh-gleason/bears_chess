#pragma once

#include "bears_chess/board.hpp"
#include "bears_chess/score.hpp"

namespace bears_chess {

int16_t evaluate_white(const Board& board);

int16_t material_difference_white(const Board& board);

template <Color side_to_move=Color::NONE>
inline int16_t evaluate(const Board& board) {
    if constexpr (side_to_move == Color::WHITE) {
        return evaluate_white(board);
    } else if constexpr (side_to_move == Color::BLACK) {
        return -evaluate_white(board);
    } else {
        if (board.side_to_move == Color::WHITE) {
            return evaluate<Color::WHITE>(board);
        }
        return evaluate<Color::BLACK>(board);
    }
}

template <Color side_to_move=Color::NONE>
int16_t material_difference(const Board& board) {
    if constexpr (side_to_move == Color::WHITE) {
        return material_difference_white(board);
    } else if constexpr (side_to_move == Color::BLACK) {
        return -material_difference_white(board);
    } else {
        if (board.side_to_move == Color::WHITE) {
            return material_difference<Color::WHITE>(board);
        }
        return material_difference<Color::BLACK>(board);
    }
}

template<Color color>
int16_t static_exchange_evaluation(const Board& board, const Move& move) {
    assert(is_capture(move.move_type));

    constexpr Color opponent_color = ~color;
    constexpr Direction dir_from = (color == Color::WHITE ? Direction::SOUTH : Direction::NORTH);
    constexpr std::array<Piece, num_of<Piece>> PIECES_BY_VALUE = {
        Piece::PAWN, Piece::KNIGHT, Piece::BISHOP, Piece::ROOK, Piece::QUEEN, Piece::KING
    };

    const Square to = move.to;
    const Bitboard bb_to = bb_square(to);

    Bitboard bb_occupied = board.occupied;

    // simulate capture
    Piece captured_piece;
    if (move.move_type == MoveType::EP_CAPTURE) {
        captured_piece = Piece::PAWN;
        bb_occupied ^= bb_shift<dir_from>(bb_to);
    } else {
        captured_piece = board.get_piece_at(to);
    }
    bb_occupied ^= bb_square(move.from);

    Piece on_square = board.get_piece_at(move.from);
    Color side_to_move = opponent_color;

    std::array<int16_t, 32> gain;
    int depth = 0;
    gain[depth] = PIECE_VALUES[idx(captured_piece)];

    // rook and bishoplike attackers need to be recomputed after every capture
    const Bitboard bb_queens =
        board.pieces[idx(Color::WHITE)][idx(Piece::QUEEN)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::QUEEN)];
    const Bitboard bb_rooklike = (
    bb_queens |
        board.pieces[idx(Color::WHITE)][idx(Piece::ROOK)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::ROOK)]);
    const Bitboard bb_bishoplike = (
        bb_queens |
        board.pieces[idx(Color::WHITE)][idx(Piece::BISHOP)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::BISHOP)]);

    // knights, king, pawns can just be masked out after occupancy changes
    const Bitboard bb_knight_attackers = bb_attacks<Piece::KNIGHT>(to) & (
        board.pieces[idx(Color::WHITE)][idx(Piece::KNIGHT)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::KNIGHT)]);
    const Bitboard bb_king_attackers = bb_attacks<Piece::KING>(to) & (
        board.pieces[idx(Color::WHITE)][idx(Piece::KING)] |
        board.pieces[idx(Color::BLACK)][idx(Piece::KING)]);
    // pawn attacks from to_square locate candidate attacking pawns of the opposite color
    const Bitboard bb_white_pawn_attackers =
        bb_attacks<Color::BLACK, Piece::PAWN>(to) &
        board.pieces[idx(Color::WHITE)][idx(Piece::PAWN)];
    const Bitboard bb_black_pawn_attackers =
        bb_attacks<Color::WHITE, Piece::PAWN>(to) &
        board.pieces[idx(Color::BLACK)][idx(Piece::PAWN)];
    const Bitboard bb_static_attackers = (
        bb_knight_attackers |
        bb_king_attackers |
        bb_white_pawn_attackers |
        bb_black_pawn_attackers
    );

    // continue simulating captures
    while (true) {
        // compute rook/bishop attackers using updated occupancy
        const Bitboard bb_all_attackers = bb_occupied & (
            bb_static_attackers |
            (bb_attacks<Piece::ROOK>(to, bb_occupied) & bb_rooklike) |
            (bb_attacks<Piece::BISHOP>(to, bb_occupied) & bb_bishoplike));
        const Bitboard bb_attackers = bb_all_attackers & board.occupied_by_color[idx(side_to_move)];
        if (zero(bb_attackers)) {
            break;
        }

        // choose next attacker
        Piece attacker = Piece::NONE;
        Bitboard bb_attacker = Bitboard::EMPTY;
        for (Piece piece : PIECES_BY_VALUE) {
            const Bitboard bb_candidates = bb_attackers & board.pieces[idx(side_to_move)][idx(piece)];
            if (nonzero(bb_candidates)) {
                attacker = piece;
                bb_attacker = lsb(bb_candidates);
                break;
            }
        }

        // king may only join if nothing can capture
        if (attacker == Piece::KING && nonzero(bb_all_attackers & board.occupied_by_color[idx(~side_to_move)])) {
            break;
        }

        ++depth;
        gain[depth] = PIECE_VALUES[idx(on_square)] - gain[depth - 1];

        // simulate capture
        on_square = attacker;
        bb_occupied ^= bb_attacker;
        side_to_move = ~side_to_move;
    }

    // either side may stop the exchange rather than continue at a loss
    while (depth > 0) {
        gain[depth - 1] = -std::max<int16_t>(-gain[depth - 1], gain[depth]);
        --depth;
    }

    return gain[0];
}


} // namespace bears_chess
