#pragma once

#include "types.hpp"
#include "bitboard.hpp"

class Board {
    public:
        Board();
        Board(const Board&) = default;
        Board& operator=(const Board&) = default;

        UndoInfo do_move(const Move& move);
        void undo_move(const UndoInfo& undo_info);
    
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
            for (Color color = Color::FIRST; color <= Color::LAST; ++color) {
                occupied_by_color[idx(color)] &= mask;
                for (Piece piece = Piece::FIRST; piece <= Piece::LAST; ++piece) {
                    pieces[idx(color)][idx(piece)] &= mask;
                }
            }
            occupied &= mask;
        }

        inline bool test_bit(Color c, Piece p, Square s) const {
            return pieces[idx(c)][idx(p)] & bb_square(s);
        }

        inline bool is_occupied(Square s) const {
            return occupied & bb_square(s);
        }

        inline bool is_occupied_by_color(Color c, Square s) const {
            return occupied_by_color[idx(c)] & bb_square(s);
        }

        inline Color get_color_at(Square s) const {
            for (Color color = Color::FIRST; color <= Color::LAST; ++color) {
                if (is_occupied_by_color(color, s)) {
                    return color;
                }
            }
            return Color::NONE;
        }

        inline std::pair<Color, Piece> get_color_piece_at(Square s) const {
            Color color = get_color_at(s);
            if (color != Color::NONE) {
                for (Piece piece = Piece::FIRST; piece <= Piece::LAST; ++piece) {
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

        Color side_to_move;
        uint16_t fullmove_number;

        CastlingRights castling_rights;
        Square ep_square;
        uint8_t halfmove_clock;

        // piece locations
        Bitboard pieces[COLOR_COUNT][PIECE_COUNT];
        Bitboard occupied_by_color[COLOR_COUNT];
        Bitboard occupied;

        // other information computed when requested
};
