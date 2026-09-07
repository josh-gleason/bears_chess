#include "board_utils.hpp"

#include "bitboard.hpp"
#include "movegen.hpp"
#include "zobrist.hpp"

#include <sstream>
#include <optional>
#include <tuple>
#include <vector>

namespace bears_chess {

constexpr long OSTREAM_WORD_INIT_BIT = 0x80000000;

constexpr BoardFormat DEFAULT_BOARD_FORMAT = BoardFormat();
constexpr Piece DEFAULT_BITBOARD_PIECE = Piece::NONE;
constexpr Color DEFAULT_BITBOARD_COLOR = Color::BLACK;

constexpr const char* PIECE_STR[num_of<Piece> * num_of<Color>] = {
    "N", "B", "R", "Q", "K", "P",
    "n", "b", "r", "q", "k", "p"
};
constexpr const char* FILE_STR[num_of<File>] = {
    "a", "b", "c", "d", "e", "f", "g", "h"
};
constexpr const char* RANK_STR[num_of<Rank>] = {
    "1", "2", "3", "4", "5", "6", "7", "8"
};
constexpr const char* SQUARE_STR[num_of<Square>] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};
constexpr const char8_t* PIECE_UTF8[(1 + num_of<Piece>) * num_of<Color>] = {
    u8"♘", u8"♗", u8"♖", u8"♕", u8"♔", u8"♙",
    u8"♞", u8"♝", u8"♜", u8"♛", u8"♚", u8"♟"
};
constexpr const char* COLOR_STR[num_of<Color>] = {
    "White", "Black"
};

constexpr const char8_t* OCCUPIED_UTF8[num_of<Color>] = {
    u8"⨉", u8"⨉"
};
constexpr const char* OCCUPIED_STR[num_of<Color>] = {
    "x", "X"
};

constexpr const char* ANSI_LIGHT_SQUARE = "\033[48;2;186;168;140m";
constexpr const char* ANSI_DARK_SQUARE = "\033[48;2;181;136;99m";
// constexpr const char* ANSI_HIGHLIGHTED_LIGHT_SQUARE = "\033[48;2;185;180;90m";
constexpr const char* ANSI_HIGHLIGHTED_LIGHT_SQUARE = "\033[48;2;208;90;105m";
constexpr const char* ANSI_HIGHLIGHTED_DARK_SQUARE = "\033[48;2;205;70;83m";
constexpr const char* ANSI_WHITE_PIECE = "\033[38;2;255;255;255m";
constexpr const char* ANSI_BLACK_PIECE = "\033[38;2;0;0;0m";

std::string color_to_str(Color color) {
    return COLOR_STR[idx(color)];
}

std::string piece_to_str(Color color, Piece piece) {
    return PIECE_STR[idx(color) * num_of<Piece> + idx(piece)];
}

std::string file_to_str(const File file) {
    return FILE_STR[idx(file)];
}

std::string rank_to_str(const Rank rank) {
    return RANK_STR[idx(rank)];
}

std::string square_to_str(const Square square) {
    return SQUARE_STR[idx(square)];
}

static std::string occupied_to_str(Color color) {
    return OCCUPIED_STR[idx(color)];
}

static std::string piece_to_utf8(Color color, Piece piece) {
    const std::u8string_view glyph = PIECE_UTF8[idx(color) * num_of<Piece> + idx(piece)];
    return std::string(glyph.begin(), glyph.end());
}

static std::string occupied_to_utf8(Color color) {
    const std::u8string_view glyph = OCCUPIED_UTF8[idx(color)];
    return std::string(glyph.begin(), glyph.end());
}

static std::tuple<Color, Piece> char_to_piece(const char ch) {
    for (Piece piece : iter<Piece>) {
        for (Color color : iter<Color>) {
            if (ch == piece_to_str(color, piece)[0]) {
                return {color, piece};
            }
        }
    }
    throw std::invalid_argument("Invalid character");
}

Board load_fen(const std::string &fen)
{
    Board board;

    std::istringstream ss(fen);
    std::string token;

    if (!std::getline(ss, token, ' ')) {
        throw std::invalid_argument("Invalid FEN: missing piece placement");
    }

    File file = first<File>;
    Rank rank = Rank::_8;
    for (char c : token) {
        if (c == '/') {
            if (file <= last<File>) {
                throw std::invalid_argument("Invalid FEN: incomplete rank");
            }
            --rank;
            file = first<File>;
        } else {
            if (std::isdigit(c)) {
                int value = c - '0';
                while (value--) {
                    if (file > last<File>) {
                        throw std::invalid_argument("Invalid FEN: overfull rank");
                    }
                    board.clear_square<false>(square_of(file, rank));
                    ++file;
                }
            } else {
                if (file > last<File>) {
                    throw std::invalid_argument("Invalid FEN: overfull rank");
                }
                auto [color, piece] = char_to_piece(c);
                Square square = square_of(file, rank);
                board.clear_square<false>(square_of(file, rank));
                if (piece != Piece::KING)
                    board.place(color, piece, square);
                else
                    board.place_king(color, square);
                ++file;
            }
        }
    }

    if (file <= last<File>) {
        throw std::invalid_argument("Invalid FEN: incomplete rank");
    }

    if (!std::getline(ss, token, ' ')) {
        throw std::invalid_argument("Invalid FEN: missing side to move");
    }

    if (token == "w") {
        board.side_to_move = Color::WHITE;
    } else if (token == "b") {
        board.side_to_move = Color::BLACK;
    } else {
        throw std::invalid_argument("Invalid FEN: invalid side to move");
    }

    if (!std::getline(ss, token, ' ')) {
        throw std::invalid_argument("Invalid FEN: missing castling rights");
    }
    board.castling_rights = CastlingRights::NONE;
    if (token != "-") {
        for (char c : token) {
            switch (c) {
                case 'K':
                    board.castling_rights |= CastlingRights::WHITE_KING;
                    break;
                case 'Q':
                    board.castling_rights |= CastlingRights::WHITE_QUEEN;
                    break;
                case 'k':
                    board.castling_rights |= CastlingRights::BLACK_KING;
                    break;
                case 'q':
                    board.castling_rights |= CastlingRights::BLACK_QUEEN;
                    break;
                default:
                    throw std::invalid_argument("Invalid FEN: invalid castling right");
            }
        }
    }

    if (!std::getline(ss, token, ' ')) {
        throw std::invalid_argument("Invalid FEN: missing en passant square");
    }

    if (token == "-") {
        board.ep_square = Square::NONE;
    } else {
        if (token.length() != 2) {
            throw std::invalid_argument("Invalid FEN: invalid en passant square format");
        }

        char file_ch = token[0];
        char rank_ch = token[1];
        if (file_ch < 'a' || file_ch > 'h') {
            throw std::invalid_argument("Invalid FEN: invalid en passant square format");
        }
        if (rank_ch < '1' || rank_ch > '8') {
            throw std::invalid_argument("Invalid FEN: invalid en passant square format");
        }

        board.ep_square = square_of(
            static_cast<File>(file_ch - 'a'),
            static_cast<Rank>(rank_ch - '1')
        );
    }

    if (!std::getline(ss, token, ' ')) {
        board.halfmove_clock = 0;
    } else {
        int clock_value;
        try {
            clock_value = std::stoi(token);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid FEN: invalid halfmove clock");
        }
        if (clock_value < 0 || clock_value > 255) {
            throw std::invalid_argument("Invalid FEN: invalid halfmove clock");
        }
        board.halfmove_clock = static_cast<uint8_t>(clock_value);
    }

    if (!std::getline(ss, token, ' ')) {
        board.fullmove_number = 1;
    } else {
        int move_number;
        try {
            move_number = std::stoi(token);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid FEN: invalid move number");
        }
        if (move_number < 1) {
            throw std::invalid_argument("Invalid FEN: invalid move number");
        }
        board.fullmove_number = move_number;
    }

    board.hash = compute_zobrist_hash(board);

    return board;
}

std::string get_fen(const Board& board)
{
    std::string fen;
    for (Rank rank : iter_rev<Rank>) {
        int empty_count = 0;
        for (File file : iter<File>) {
            Square square = square_of(file, rank);
            Piece piece = board.get_piece_at<false>(square);
            Color color = board.get_color_at<false>(square);

            if (piece == Piece::NONE) {
                ++empty_count;
            } else {
                if (empty_count) {
                    fen += std::to_string(empty_count);
                }
                fen += piece_to_str(color, piece);
                empty_count = 0;
            }
        }

        if (empty_count) {
            fen += std::to_string(empty_count);
        }
        if (rank != first<Rank>) {
            fen += "/";
        }
    }

    if (board.side_to_move == Color::WHITE) {
        fen += " w";
    } else {
        fen += " b";
    }

    fen += " ";
    if (board.castling_rights == CastlingRights::NONE) {
        fen += "-";
    } else {
        if ((board.castling_rights & CastlingRights::WHITE_KING) != CastlingRights::NONE) {
            fen += "K";
        }
        if ((board.castling_rights & CastlingRights::WHITE_QUEEN) != CastlingRights::NONE) {
            fen += "Q";
        }
        if ((board.castling_rights & CastlingRights::BLACK_KING) != CastlingRights::NONE) {
            fen += "k";
        }
        if ((board.castling_rights & CastlingRights::BLACK_QUEEN) != CastlingRights::NONE) {
            fen += "q";
        }
    }

    fen += " ";
    if (board.ep_square == Square::NONE) {
        fen += "-";
    } else {
        fen += square_to_str(board.ep_square);
    }

    fen += " " + std::to_string(board.halfmove_clock) + " " + std::to_string(board.fullmove_number);

    return fen;
}

static int get_stream_board_fmt_idx() {
    static int idx = std::ios_base::xalloc();
    return idx;
}

static int get_stream_bitboard_color_fmt_idx() {
    static int idx = std::ios_base::xalloc();
    return idx;
}

static int get_stream_bitboard_piece_fmt_idx() {
    static int idx = std::ios_base::xalloc();
    return idx;
}

static std::string get_piece_glyph(Color color, Piece piece, BoardFormat fmt) {
    if (piece == Piece::NONE) {
        return (check_flag(fmt, BoardFormat::NO_COLOR) ? "." : " ");
    } else if (check_flag(fmt, BoardFormat::NO_UNICODE)) {
        return piece_to_str(color, piece);
    } else if (check_flag(fmt, BoardFormat::NO_COLOR)) {
        return piece_to_utf8(color, piece);
    } else {
        // foreground color handled by ANSI escape codes
        return piece_to_utf8(Color::BLACK, piece);
    }
}

static std::string get_piece_occupancy_glyph(Color color, bool occupied, BoardFormat fmt) {
    if (!occupied) {
        return (check_flag(fmt, BoardFormat::NO_COLOR) ? "." : " ");
    } else if (check_flag(fmt, BoardFormat::NO_UNICODE)) {
        return occupied_to_str(color);
    } else if (check_flag(fmt, BoardFormat::NO_COLOR)) {
        return occupied_to_utf8(color);
    } else {
        // foreground color handled by ANSI escape codes
        return occupied_to_utf8(Color::BLACK);
    }
}

static std::string get_ansi_code(Square square, Color color, BoardFormat fmt, bool highlighted) {
    if (check_flag(fmt, BoardFormat::NO_COLOR)) {
        return "";
    }

    std::string code = "";
    code += (
        is_light(square) ?
        (highlighted ? ANSI_HIGHLIGHTED_LIGHT_SQUARE : ANSI_LIGHT_SQUARE) :
        (highlighted ? ANSI_HIGHLIGHTED_DARK_SQUARE : ANSI_DARK_SQUARE)
    );
    code += (color == Color::WHITE ? ANSI_WHITE_PIECE : ANSI_BLACK_PIECE);
    return code;
}

static std::string get_ansi_reset(BoardFormat fmt) {
    constexpr const char* ANSI_RESET = "\033[0m";
    if (check_flag(fmt, BoardFormat::NO_COLOR)) {
        return "";
    }
    return ANSI_RESET;
}

static std::string get_square_str(Color color, Piece piece, Square square, BoardFormat fmt, bool highlighted=false) {
    return get_ansi_code(square, color, fmt, highlighted) + get_piece_glyph(color, piece, fmt) + " " + get_ansi_reset(fmt);
}

static std::string get_square_occupancy_str(Color color, bool occupied, Square square, BoardFormat fmt, bool highlighted=false) {
    return get_ansi_code(square, color, fmt, highlighted) + get_piece_occupancy_glyph(color, occupied, fmt) + " " + get_ansi_reset(fmt);
}

static std::vector<Rank> get_rank_order(BoardFormat fmt) {
    std::vector<Rank> ranks;
    ranks.reserve(num_of<Rank>);
    if (check_flag(fmt, BoardFormat::ORIENT_BLACK)) {
        for (Rank rank : iter<Rank>) {
            ranks.push_back(rank);
        }
    } else {
        for (Rank rank : iter_rev<Rank>) {
            ranks.push_back(rank);
        }
    }
    return ranks;
}

static std::vector<File> get_file_order(BoardFormat fmt) {
    std::vector<File> files;
    files.reserve(num_of<File>);
    if (check_flag(fmt, BoardFormat::ORIENT_BLACK)) {
        for (File file : iter_rev<File>) {
            files.push_back(file);
        }
    } else {
        for (File file : iter<File>) {
            files.push_back(file);
        }
    }
    return files;
}

static std::string get_files_string(BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_LABELS)) {
        return "";
    }

    std::string file_labels = " ";
    for (File file : get_file_order(fmt)) {
        file_labels += std::string(" ") + file_to_str(file);
    }
    return file_labels;
}

static std::string board_rank_string(const Board& board, Rank rank, BoardFormat fmt, Bitboard highlights = Bitboard::EMPTY) {
    std::string rank_string;
    if (!check_flag(fmt, BoardFormat::HIDE_LABELS)) {
        rank_string += std::string(rank_to_str(rank)) + " ";
    }
    for (File file : get_file_order(fmt)) {
        Square square = square_of(file, rank);
        Color color = board.get_color_at<false>(square);
        bool highlighted = nonzero(highlights & bb_square(square));
        if (check_flag(fmt, BoardFormat::OCCUPANCY_ONLY)) {
            bool occupied = board.is_occupied(square);
            rank_string += get_square_occupancy_str(color, occupied, square, fmt, highlighted);
        } else {
            Piece piece = board.get_piece_at<false>(square);
            rank_string += get_square_str(color, piece, square, fmt, highlighted);
        }
    }
    return rank_string;
}

static std::string full_move_string(const Board& board, BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_FULLMOVE_NUMBER)) {
        return "";
    } else {
        return std::string("Move number ") + std::to_string(board.fullmove_number);
    }
}

static std::string turn_string(const Board& board, BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_TURN)) {
        return "";
    } else {
        return (
            color_to_str(board.side_to_move) +
            std::string(" to play")
        );
    }
}

static std::string castling_rights_string(const Board& board, BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_CASTLING_RIGHTS)) {
        return "";
    } else {
        std::string str = "Castling rights: ";
        if (check_flag(board.castling_rights, CastlingRights::WHITE_KING)) {
            str += get_square_str(Color::WHITE, Piece::KING, Square::E1, fmt);
        }
        if (check_flag(board.castling_rights, CastlingRights::WHITE_QUEEN)) {
            str += get_square_str(Color::WHITE, Piece::QUEEN, Square::D1, fmt);
        }
        if (check_flag(board.castling_rights, CastlingRights::BLACK_KING)) {
            str += get_square_str(Color::BLACK, Piece::KING, Square::E8, fmt);
        }
        if (check_flag(board.castling_rights, CastlingRights::BLACK_QUEEN)) {
            str += get_square_str(Color::BLACK, Piece::QUEEN, Square::D8, fmt);
        }
        if (board.castling_rights == CastlingRights::NONE) {
            str += "None";
        }
        return str;
    }
}

static std::string halfmove_clock_string(const Board& board, BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_HALFMOVE_CLOCK)) {
        return "";
    } else {
        return (
            std::string("Halfmove clock: ") +
            std::to_string(board.halfmove_clock) +
            std::string(board.halfmove_clock == 1 ? " ply" : " plies")
        );
    }
}

static std::string ep_square_string(const Board& board, BoardFormat fmt) {
    if (check_flag(fmt, BoardFormat::HIDE_EP_SQUARE)) {
        return "";
    } else {
        return (
            board.ep_square == Square::NONE ?
            std::string("En passant not possible") :
            std::string("En passant allowed on ") + square_to_str(board.ep_square)
        );
    }
}

template<typename T>
inline void set_ostream_word(std::ostream &out, int idx, T word) {
    long iword = static_cast<long>(word);
    // msb is reserved for initialization status
    assert((iword & OSTREAM_WORD_INIT_BIT) != OSTREAM_WORD_INIT_BIT);
    out.iword(idx) = iword | OSTREAM_WORD_INIT_BIT;
}

void set_stream_board_fmt(std::ostream &out, BoardFormat format) {
    set_ostream_word(out, get_stream_board_fmt_idx(), format);
}

void set_stream_bitboard_piece_fmt(std::ostream &out, Piece piece) {
    set_ostream_word(out, get_stream_bitboard_piece_fmt_idx(), piece);    
}

void set_stream_bitboard_color_fmt(std::ostream &out, Color color) {
    set_ostream_word(out, get_stream_bitboard_color_fmt_idx(), color);    
}

template<typename T>
inline T get_ostream_word(std::ostream &out, int idx, std::optional<T> _default = std::nullopt) {
    long iword = out.iword(idx);
    if (iword & OSTREAM_WORD_INIT_BIT) {
        return static_cast<T>(iword & ~OSTREAM_WORD_INIT_BIT);
    } else {
        if (_default.has_value()) {
            return *_default;
        } else {
            // assume default constructor
            return T();
        }
    }
}

BoardFormat get_board_fmt(std::ostream &out) {
    return get_ostream_word<BoardFormat>(out, get_stream_board_fmt_idx(), DEFAULT_BOARD_FORMAT);
}

Piece get_bitboard_fmt_piece(std::ostream &out) {
    return get_ostream_word<Piece>(out, get_stream_bitboard_piece_fmt_idx(), DEFAULT_BITBOARD_PIECE);
}

Color get_bitboard_fmt_color(std::ostream &out) {
    return get_ostream_word<Color>(out, get_stream_bitboard_color_fmt_idx(), DEFAULT_BITBOARD_COLOR);
}

namespace detail {
    std::ostream& operator<<(std::ostream &out, BoardFormatFlags fmt) {
        set_stream_board_fmt(out, fmt.flags);
        return out;
    }

    std::ostream &operator<<(std::ostream &out, BitboardFormatFlags fmt)
    {
        set_stream_bitboard_color_fmt(out, fmt.color);
        set_stream_bitboard_piece_fmt(out, fmt.piece);
        return out;
    }

    std::ostream &operator<<(std::ostream &out, const Highlighted<Board> &highlighted_board)
    {
        constexpr int FULL_MOVE_LINE = 1;
        constexpr int TURN_LINE = 2;
        constexpr int CASTLING_RIGHTS_LINE = 4;
        constexpr int HALF_MOVE_LINE = 5;
        constexpr int EP_SQUARE_LINE = 6;
        
        bool first_line = true;

        const Board& board = highlighted_board.obj;
        Bitboard highlights = highlighted_board.highlights;

        BoardFormat fmt = get_board_fmt(out);
        
        if (check_flag(fmt, BoardFormat::ORIENT_TURN)) {
            fmt = clear_flag(fmt, BoardFormat::ORIENT_TURN);
            fmt = (board.side_to_move == Color::WHITE) ? clear_flag(fmt, BoardFormat::ORIENT_BLACK) : set_flag(fmt, BoardFormat::ORIENT_BLACK);
        }

        if (!check_flag(fmt, BoardFormat::HIDE_FEN)) {
            out << get_fen(board);
            first_line = false;
        }

        auto ranks = get_rank_order(fmt);

        if (check_flag(fmt, BoardFormat::HIDE_BOARD)) {
            for (size_t rank_idx = 0; rank_idx < ranks.size(); ++rank_idx) {
                std::string info = "";
                if (rank_idx == FULL_MOVE_LINE) {
                    info = full_move_string(board, fmt);
                } else if (rank_idx == TURN_LINE) {
                    info = turn_string(board, fmt);
                } else if (rank_idx == CASTLING_RIGHTS_LINE) {
                    info = castling_rights_string(board, fmt);
                } else if (rank_idx == HALF_MOVE_LINE) {
                    info = halfmove_clock_string(board, fmt);
                } else if (rank_idx == EP_SQUARE_LINE) {
                    info = ep_square_string(board, fmt);
                }
                if (info.size() > 0) {
                    out << (first_line ? "" : "\n") << info;
                    first_line = false;
                }
            }
        } else {
            for (size_t rank_idx = 0; rank_idx < ranks.size(); ++rank_idx) {
                out << (first_line ? "" : "\n");
                first_line = false;
                out << board_rank_string(board, ranks[rank_idx], fmt, highlights);

                if (rank_idx == FULL_MOVE_LINE) {
                    out << "    " << full_move_string(board, fmt);
                } else if (rank_idx == TURN_LINE) {
                    out << "    " << turn_string(board, fmt);
                } else if (rank_idx == CASTLING_RIGHTS_LINE) {
                    out << "    " << castling_rights_string(board, fmt);
                } else if (rank_idx == HALF_MOVE_LINE) {
                    out << "    " << halfmove_clock_string(board, fmt);
                } else if (rank_idx == EP_SQUARE_LINE) {
                    out << "    " << ep_square_string(board, fmt);
                }
            }
            out << "\n" << get_files_string(fmt);
        }

        return out;
    }

    std::ostream &operator<<(std::ostream &out, const Highlighted<Bitboard> &highlighted_bb)
    {
        BoardFormat prev_board_fmt = get_board_fmt(out);
        BoardFormat new_board_fmt = prev_board_fmt;

        Color color = get_bitboard_fmt_color(out);
        Piece piece = get_bitboard_fmt_piece(out);

        Bitboard bb = highlighted_bb.obj;
        Bitboard highlights = highlighted_bb.highlights;

        if (color == Color::NONE) {
            color = Color::BLACK;
        }
        if (piece == Piece::NONE) {
            piece = Piece::PAWN;
            new_board_fmt |= BoardFormat::OCCUPANCY_ONLY;
        }
        new_board_fmt |= BoardFormat::ONLY_BOARD;

        Board board;
        for (Square square : iter<Square>) {
            board.clear_square<false>(square);
        }

        for (Square square : BBSquareScan(bb)) {
            if (piece != Piece::KING)
                board.place(color, piece, square);
            else
                board.place_king(color, square);
        }

        return out << set_board_format(new_board_fmt) << show_highlights(board, highlights) << set_board_format(prev_board_fmt);
    }
}

std::ostream& operator<<(std::ostream& out, const Board& board) {
    return out << show_highlights(board, Bitboard::EMPTY);
}

std::ostream& operator<<(std::ostream& out, Bitboard bb) {
    return out << show_highlights(bb, Bitboard::EMPTY);
}

std::ostream& operator<<(std::ostream& out, Square square) {
    return out << square_to_str(square);
}

std::ostream& operator<<(std::ostream& out, Rank rank) {
    return out << rank_to_str(rank);
}

std::ostream& operator<<(std::ostream& out, File file) {
    return out << file_to_str(file);
}

std::ostream& operator<<(std::ostream& out, Piece piece) {
    return out << piece_to_str(Color::BLACK, piece);
}

std::ostream& operator<<(std::ostream& out, Color color) {
    return out << color_to_str(color);
}

std::ostream& operator<<(std::ostream& out, MoveType move_type) {
    switch (move_type) {
        case MoveType::QUIET:
            out << "quiet";
            break;
        case MoveType::DOUBLE_PAWN_PUSH:
            out << piece_to_str(Color::WHITE, Piece::PAWN) << "x2";
            break;
        case MoveType::KING_CASTLE:
            out << "O-O";
            break;
        case MoveType::QUEEN_CASTLE:
            out << "O-O-O";
            break;
        case MoveType::CAPTURE:
            out << "x";
            break;
        case MoveType::EP_CAPTURE:
            out << "x-ep";
            break;
        case MoveType::KNIGHT_PROMOTION:
            out << "=" << piece_to_str(Color::WHITE, Piece::KNIGHT);
            break;
        case MoveType::BISHOP_PROMOTION:
            out << "=" << piece_to_str(Color::WHITE, Piece::BISHOP);
            break;
        case MoveType::ROOK_PROMOTION:
            out << "=" << piece_to_str(Color::WHITE, Piece::ROOK);
            break;
        case MoveType::QUEEN_PROMOTION:
            out << "=" << piece_to_str(Color::WHITE, Piece::QUEEN);
            break;
        case MoveType::KNIGHT_PROMOTION_CAPTURE:
            out << "x=" << piece_to_str(Color::WHITE, Piece::KNIGHT);
            break;
        case MoveType::BISHOP_PROMOTION_CAPTURE:
            out << "x=" << piece_to_str(Color::WHITE, Piece::BISHOP);
            break;
        case MoveType::ROOK_PROMOTION_CAPTURE:
            out << "x=" << piece_to_str(Color::WHITE, Piece::ROOK);
            break;
        case MoveType::QUEEN_PROMOTION_CAPTURE:
            out << "x=" << piece_to_str(Color::WHITE, Piece::QUEEN);
            break;
        case MoveType::NONE:
            out << "none";
            break;
        default:
            out << "MoveType(" << static_cast<int>(move_type) << ")";
            break;
    };
    return out;
}

std::ostream& operator<<(std::ostream& out, Move move) {
    return out << move.from << "->" << move.to << " " << move.move_type;
}

File parse_file(char f) {
    if (f >= FILE_STR[0][0] && f <= FILE_STR[num_of<File> - 1][0]) {
        return static_cast<File>(f - FILE_STR[0][0]);
    }
    throw std::invalid_argument(std::format("Invalid file {}", f));
}

Rank parse_rank(char r) {
    if (r >= RANK_STR[0][0] && r <= RANK_STR[num_of<Rank> - 1][0]) {
        return static_cast<Rank>(r - RANK_STR[0][0]);
    }
    throw std::invalid_argument(std::format("Invalid rank {}", r));
}

Square parse_square(char f, char r) {
    return square_of(parse_file(f), parse_rank(r));
}

Piece parse_piece(char p) {
    for (Piece piece : iter<Piece>) {
        if (p == piece_to_str(Color::BLACK, piece)[0]) {
            return piece;
        }
    }
    throw std::invalid_argument(std::format("Invalid piece {}", p));
}

Move convert_uci_to_move(const std::string& uci_move, const Board& board) {
    MoveList legal_moves = generate_moves<LegalPolicy>(board);
    if (uci_move.size() != 4 && uci_move.size() != 5) {
        throw std::invalid_argument(std::format("Move {} is invalid", uci_move));
    }
    Square from = parse_square(uci_move[0], uci_move[1]);
    Square to = parse_square(uci_move[2], uci_move[3]);
    Piece promotion = Piece::NONE;
    if (uci_move.size() == 5) {
        promotion = parse_piece(uci_move[4]);
    }

    for (const auto& move : legal_moves) {
        if (
            move.from == from &&
            move.to == to && (
                (!is_promotion(move.move_type) && promotion == Piece::NONE) ||
                (is_promotion(move.move_type) && promote_to(move.move_type) == promotion)
            )
        ) {
            return move;
        }
    }
    throw std::invalid_argument(std::format("Invalid Move {}", uci_move));
}

const std::string convert_move_to_uci(const Move& move) {
    std::string result;
    result = square_to_str(move.from) + square_to_str(move.to);
    if (is_promotion(move.move_type)) {
        result += piece_to_str(Color::BLACK, promote_to(move.move_type));
    }
    return result;
}

} // namespace bears_chess
