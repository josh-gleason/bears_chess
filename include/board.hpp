#pragma once

#include "types.hpp"
#include "bitboard.hpp"
#include "magics.hpp"

namespace bears_chess {

class Board {
    public:
        Board();
        Board(const Board&) = default;
        Board& operator=(const Board&) = default;
        bool operator==(const Board&) const = default;
        bool operator!=(const Board&) const = default;

        UndoInfo do_move(const Move& move);
        void undo_move(const UndoInfo& undo_info);
        bool is_legal(const Move& last_move) const;
    
        // utility functions

        inline void place(Color c, Piece p, Square s) {
            last_color_sq[idx(s)] = c;
            last_piece_sq[idx(s)] = p;

            pieces[idx(c)][idx(p)] |= bb_square(s);
            occupied_by_color[idx(c)] |= bb_square(s);
            occupied |= bb_square(s);
        }

        inline void remove(Color c, Piece p, Square s) {
            pieces[idx(c)][idx(p)] &= ~bb_square(s);
            occupied_by_color[idx(c)] &= ~bb_square(s);
            occupied &= ~bb_square(s);
        }

        template<bool assume_occupied=true>
        inline void clear_square(Square s) {
            if constexpr (!assume_occupied) {
                if (!is_occupied(s))
                    return;
            }
            remove(last_color_sq[idx(s)], last_piece_sq[idx(s)], s);
        }

        template<bool assume_occupied=true>
        inline void clear_square_of_color(Color c, Square s) {
            if constexpr (!assume_occupied) {
                if (!is_occupied(s))
                    return;
            }
            Piece piece = last_piece_sq[idx(s)];
            remove(c, last_piece_sq[idx(s)], s);
        }

        inline bool test_bit(Color c, Piece p, Square s) const {
            return nonzero(pieces[idx(c)][idx(p)] & bb_square(s));
        }

        inline bool is_occupied(Square s) const {
            return nonzero(occupied & bb_square(s));
        }

        inline bool is_occupied_by_color(Color c, Square s) const {
            return nonzero(occupied_by_color[idx(c)] & bb_square(s));
        }

        template<bool assume_occupied=true>
        inline Color get_color_at(Square s) const {
            if constexpr (!assume_occupied) {
                if (!is_occupied(s))
                    return Color::NONE;
            }
            return last_color_sq[idx(s)];
        }

        template<bool assume_occupied=true>
        inline Piece get_piece_at(Square s) const {
            if constexpr (!assume_occupied) {
                if (!is_occupied(s))
                    return Piece::NONE;
            }
            return last_piece_sq[idx(s)];
        }

        template<bool assume_occupied=true>
        inline Piece get_piece_of_color_at(Color c, Square s) const {
            if constexpr (assume_occupied) {
                return get_piece_at(s);
            } else {
                return (is_occupied_by_color(c, s) ? get_piece_at(s) : Piece::NONE);
            }
        }

        inline bool is_square_attacked(Square sq, Color by) const {
            if (nonzero(pieces[idx(by)][idx(Piece::PAWN)] & BB_CAPTURE_PAWN_MOVES[idx(~by)][idx(sq)]))
                return true;
            if (nonzero(pieces[idx(by)][idx(Piece::KNIGHT)] & BB_KNIGHT_MOVES[idx(sq)]))
                return true;
            if (nonzero(pieces[idx(by)][idx(Piece::KING)] & BB_KING_MOVES[idx(sq)]))
                return true;
            Bitboard bb_rook_attackers = pieces[idx(by)][idx(Piece::ROOK)] | pieces[idx(by)][idx(Piece::QUEEN)];
            if (nonzero(bb_rook_attackers & magic_lookup<Piece::ROOK>(sq, occupied)))
                return true;
            Bitboard bb_bishop_attackers = pieces[idx(by)][idx(Piece::BISHOP)] | pieces[idx(by)][idx(Piece::QUEEN)];
            if (nonzero(bb_bishop_attackers & magic_lookup<Piece::BISHOP>(sq, occupied)))
                return true;
            return false;
        }

        Color side_to_move;
        uint16_t fullmove_number;

        CastlingRights castling_rights;
        Square ep_square;
        uint8_t halfmove_clock;

        // piece locations
        Bitboard pieces[num_of<Color>][num_of<Piece>];
        Bitboard occupied_by_color[num_of<Color>];
        Bitboard occupied;

        // mailbox cache
        Piece last_piece_sq[num_of<Square>];
        Color last_color_sq[num_of<Square>];
};

} // namespace bears_chess
