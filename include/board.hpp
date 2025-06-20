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

        // piece locations
        Bitboard pieces[COLOR_COUNT][PIECE_COUNT];
        Bitboard occupied_by_color[COLOR_COUNT];
        Bitboard occupied;

        Color side_to_move;
        uint16_t fullmove_number;

        CastlingRights castling_rights;
        Square ep_square;
        uint8_t halfmove_clock;
    
        // utility functions
        inline void place(Color c, Piece p, Square s) {
            pieces[idx(c)][idx(p)] |= bb_square(s);
        }

        inline void remove(Color c, Piece p, Square s) {
            pieces[idx(c)][idx(p)] &= ~bb_square(s);
        }

        inline void clear_square(Square s) {
            remove(Color::WHITE, Piece::PAWN, s);
            remove(Color::WHITE, Piece::KNIGHT, s);
            remove(Color::WHITE, Piece::BISHOP, s);
            remove(Color::WHITE, Piece::ROOK, s);
            remove(Color::WHITE, Piece::QUEEN, s);
            remove(Color::WHITE, Piece::KING, s);
            remove(Color::BLACK, Piece::PAWN, s);
            remove(Color::BLACK, Piece::KNIGHT, s);
            remove(Color::BLACK, Piece::BISHOP, s);
            remove(Color::BLACK, Piece::ROOK, s);
            remove(Color::BLACK, Piece::QUEEN, s);
            remove(Color::BLACK, Piece::KING, s);
        }

        inline bool test_bit(Color c, Piece p, Square s) const {
            return pieces[idx(c)][idx(p)] & bb_square(s);
        }

        inline Piece get_piece_at(Square s) const {
            for (Color color = Color::FIRST; color < Color::UB; ++color) {
                for (Piece piece = Piece::FIRST; piece < Piece::UB; ++piece) {
                    if (test_bit(color, piece, s)) {
                        return piece;
                    }
                }
            }
            return Piece::NONE;
        }

        inline Color get_color_at(Square s) const {
            for (Color color = Color::FIRST; color < Color::UB; ++color) {
                for (Piece piece = Piece::FIRST; piece < Piece::UB; ++piece) {
                    if (test_bit(color, piece, s)) {
                        return color;
                    }
                }
            }
            return Color::NONE;
        }
};
