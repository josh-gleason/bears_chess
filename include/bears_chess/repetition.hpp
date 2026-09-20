#pragma once

#include "bears_chess/types.hpp"
#include "bears_chess/score.hpp"

#include <span>
#include <algorithm>

namespace bears_chess {

constexpr int MAX_GAME_WINDOW = 256;

template <bool single_search_repetition = true>
inline bool is_repetition(
    std::span<const ZobristHash> root_history,
    std::span<const ZobristHash> search_history,
    ZobristHash current,
    int halfmove_clock
) {
    const int history_size = static_cast<int>(root_history.size());
    const int search_size = static_cast<int>(search_history.size());

    int matches = 0;
    const int oldest_search_index = std::max(search_size - halfmove_clock, 0);
    for (int i = search_size - 2; i >= oldest_search_index; i -= 2) {
        if constexpr (single_search_repetition) {
            // treat any repetition within search as a draw for efficiency
            if (search_history[i] == current) {
                return true;
            }
        } else {
            if (search_history[i] == current && ++matches >= 2) {
                return true;
            }
        }
    }

    const int oldest_root_index = std::max(history_size + search_size - halfmove_clock, 0);
    // start with latest historical hash for same side to move as current
    const int newest_root_index = history_size - 2 + (search_size % 2);
    for (int i = newest_root_index; i >= oldest_root_index; i -= 2) {
        if (root_history[i] == current && ++matches >= 2) {
            return true;
        }
    }
    return false;
}

class RepetitionHashes {
public:
    RepetitionHashes() {}

    RepetitionHashes(std::span<const ZobristHash> game_window, uint8_t halfmove_clock) {
        seed(game_window, halfmove_clock);
    }

    inline void seed(std::span<const ZobristHash> game_window, uint8_t halfmove_clock) {
        // only copy history since the last irreversable move
        const size_t usable_count = std::min({
            game_window.size(),
            static_cast<size_t>(halfmove_clock),
            static_cast<size_t>(MAX_GAME_WINDOW)
        });
        root_hashes.assign(game_window.last(usable_count));
    }

    inline void clear() {
        root_hashes.clear();
    }

    inline bool record_and_check(int ply, uint8_t halfmove_clock, ZobristHash hash) {
        assert(ply < MAX_PLY);
        search_hashes[ply] = hash;
        return is_repetition(
            root_hashes.view(),
            std::span(search_hashes).first(ply),
            hash,
            halfmove_clock
        );
    }

private:
    StaticVector<ZobristHash, MAX_GAME_WINDOW> root_hashes{};
    std::array<ZobristHash, MAX_PLY> search_hashes{};
};

} // namespace bears_chess
