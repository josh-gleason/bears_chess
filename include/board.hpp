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
            pieces[idx(c)][idx(p)] |= bb_square(s);
            occupied_by_color[idx(c)] |= bb_square(s);
            occupied |= bb_square(s);
        }

        inline void remove(Color c, Piece p, Square s) {
            pieces[idx(c)][idx(p)] &= ~bb_square(s);
            occupied_by_color[idx(c)] &= ~bb_square(s);
            occupied &= ~bb_square(s);
        }

        inline void clear_square(Square s) {
            Bitboard mask = ~bb_square(s);
            for (Color color : iter<Color>) {
                occupied_by_color[idx(color)] &= mask;
                for (Piece piece : iter<Piece>) {
                    pieces[idx(color)][idx(piece)] &= mask;
                }
            }
            occupied &= mask;
        }

        inline void clear_square_of_color(Color c, Square s) {
            Bitboard mask = ~bb_square(s);
            occupied_by_color[idx(c)] &= mask;
            for (Piece piece : iter<Piece>) {
                pieces[idx(c)][idx(piece)] &= mask;
            }
            occupied &= mask;
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

        inline Color get_color_at(Square s) const {
            for (Color color : iter<Color>) {
                if (is_occupied_by_color(color, s)) {
                    return color;
                }
            }
            return Color::NONE;
        }

        inline Piece get_piece_of_color_at(Color c, Square s) const {
            for (Piece piece : iter<Piece>) {
                if (test_bit(c, piece, s)) {
                    return piece;
                }
            }
            return Piece::NONE;
        }

        inline std::pair<Color, Piece> get_color_piece_at(Square s) const {
            Color color = get_color_at(s);
            if (color != Color::NONE) {
                for (Piece piece : iter<Piece>) {
                    if (test_bit(color, piece, s)) {
                        return std::make_pair(color, piece);
                    }
                }
            }
            return std::make_pair(Color::NONE, Piece::NONE);
        }

        inline Piece get_piece_at(Square s) const {
            auto [color, piece] = get_color_piece_at(s);
            return piece;
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

        // other information computed when requested
};

} // namespace bears_chess
