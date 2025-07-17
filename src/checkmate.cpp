#include "evaluation_utils.hpp"

namespace bears_chess {

// bool is_checkmate(const Board& board) {
//     MoveList unused;
//     BoardCache cache;
//     cache.opponent_attacks = calculate_opponent_attacks<true>(board);
//     Bitboard checkers = calculate_checkers<true>(board, cache.opponent_attacks);
//     int num_checkers = popcount(checkers);

//     if (num_checkers == 2) {
//         return !generate_legal_king_moves<true>(board, unused, cache);
//     } else if (num_checkers == 1) {
//         if (generate_legal_king_moves<true>(board, unused, cache))
//             return false;
//         if (generate_legal_castle_moves<true>(board, unused, cache))
//             return false;
//         cache.block_mask = checkers;
//         cache.pinned_pieces = (
//             calculate_pinned_pieces<Piece::ROOK>(board, cache.pin_masks, cache.block_mask)
//             | calculate_pinned_pieces<Piece::BISHOP>(board, cache.pin_masks, cache.block_mask)
//         );
//         if (generate_legal_knight_moves<true>(board, unused, cache))
//             return false;
//         if (generate_legal_slider_moves<Piece::ROOK, true>(board, unused, cache))
//             return false;
//         if (generate_legal_slider_moves<Piece::BISHOP, true>(board, unused, cache))
//             return false;
//         if (generate_legal_pawn_moves<true>(board, unused, cache))
//             return false;
//         return true;
//     }
//     return false;
// }

} // namespace bears_chess