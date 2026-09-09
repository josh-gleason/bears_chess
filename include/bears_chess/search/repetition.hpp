#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/score.hpp"

#include <span>
#include <algorithm>

namespace bears_chess {

constexpr int MAX_GAME_WINDOW = 256;
constexpr int MAX_REPETITION_HASHES = MAX_PLY + MAX_GAME_WINDOW;

class RepetitionHashes {
public:
    typedef std::array<ZobristHash, MAX_REPETITION_HASHES> HashList;

    inline void seed(std::span<const ZobristHash> game_window, uint8_t halfmove_clock) {
        // only copy history since the last irreversable move
        const size_t usable_count = std::min({
            game_window.size(),
            static_cast<size_t>(halfmove_clock),
            static_cast<size_t>(MAX_GAME_WINDOW)
        });
        std::copy(game_window.end() - usable_count, game_window.end(), hashes.begin());

        // root offset corresponding to hash for current game position
        root_offset = static_cast<int>(usable_count);
    }

    inline void clear() {
        root_offset = 0;
    }

    inline bool record_and_check(int ply, uint8_t halfmove_clock, ZobristHash hash) {
        const int current_index = root_offset + ply;
        hashes[current_index] = hash;

        // anything before the halfmove_clock is an impossible position to revisit, no need to check
        const int oldest_index = std::max(current_index - halfmove_clock, 0);

        // only check hashes for same side to move (step size 2)
        int matches = 0;
        for (int compare_index = current_index - 2; compare_index >= oldest_index; compare_index -= 2) {
            if (hashes[compare_index] == hash && (compare_index >= root_offset || ++matches >= 2)) {
                return true;
            }
        }
        return false;
    }

private:
    HashList hashes{};
    int root_offset{};
};

} // namespace bears_chess
