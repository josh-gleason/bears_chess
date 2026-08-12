#pragma once

#include "types.hpp"
#include "score.hpp"
#include <optional>
#include <utility>
#include <vector>

namespace bears_chess {

enum class Bound : int8_t {
    UPPER = -1,
    EXACT = 0,
    LOWER = 1,
    NONE = 2
};

struct TTEntry {
    uint32_t key;
    int32_t ibv_score;
    int16_t depth;
    uint16_t age;
    Move best_move;
};

struct TTHit {
    int16_t score;
    int16_t depth;
    Move best_move;
    Bound bound;
};


inline int32_t encode_ibv(int16_t score, Bound bound) {
    return 4 * score + static_cast<int32_t>(bound);
}


inline std::pair<int16_t, Bound> decode_ibv(int32_t ibv_score) {
    int16_t score = static_cast<int16_t>((ibv_score + 1) >> 2);
    Bound bound = static_cast<Bound>(ibv_score - 4 * score);
    return std::make_pair(score, bound);
}


class TranspositionTable {
public:
    TranspositionTable(size_t megabytes) {
        resize(megabytes);
    }

    std::optional<TTHit> probe(ZobristHash hash, int ply) const {
        const TTEntry& entry = entries[index_of(hash)];
        if (entry.key != key_of(hash)) {
            return std::nullopt;
        }

        auto [stored, bound] = decode_ibv(entry.ibv_score);

        int score = stored;
        if (score >= MATE_SCORE_BOUND) {
            score -= ply;
        } else if (score <= -MATE_SCORE_BOUND) {
            score += ply;
        }

        return TTHit{static_cast<int16_t>(score), entry.depth, entry.best_move, bound};
    }

    void store(ZobristHash hash, Move best_move, int16_t score, int depth, Bound bound, int ply) {
        TTEntry& entry = entries[index_of(hash)];

        if (entry.age == current_age && depth < entry.depth) {
            return;
        }

        // adjust mate-in-# based on current ply
        int adjusted = score;
        if (adjusted >= MATE_SCORE_BOUND) {
            adjusted += ply;
        } else if (adjusted <= -MATE_SCORE_BOUND) {
            adjusted -= ply;
        }

        entry.key = key_of(hash);
        entry.ibv_score = encode_ibv(static_cast<int16_t>(adjusted), bound);
        entry.depth = static_cast<int16_t>(depth);
        entry.age = current_age;
        entry.best_move = best_move;
    }
    

    void resize(size_t megabytes) {
        size_t num_entries = std::max<size_t>(1, std::bit_floor((megabytes << 20) / sizeof(TTEntry)));
        entries.assign(num_entries, TTEntry{});
        index_mask = static_cast<ZobristHash>(num_entries - 1);
    }

    void clear() {
        std::fill(entries.begin(), entries.end(), TTEntry{});
    }

    void new_search() {
        ++current_age;
    }

    int hashfull() const {
        // sample first 1000 entries and return number that are used for current search
        int used = 0;
        size_t sample = std::min<size_t>(1000, entries.size());
        for (size_t i = 0; i < sample; ++i) {
            if (entries[i].age == current_age) {
                ++used;
            }
        }
        return sample > 0 ? static_cast<int>(used * 1000 / sample) : 0;
    }

private:
    inline size_t index_of(ZobristHash hash) const {
        return static_cast<size_t>(index_mask & hash);
    }

    inline uint32_t key_of(ZobristHash hash) const {
        return static_cast<uint32_t>(hash >> 32);
    }

    uint16_t current_age{1};
    ZobristHash index_mask{0};
    std::vector<TTEntry> entries;
};

} // namespace bears_chess