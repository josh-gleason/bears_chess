#pragma once

#include "bears_chess/board.hpp"
#include "bears_chess/uci_options.hpp"
#include "bears_chess/search.hpp"

#include <map>
#include <thread>

namespace bears_chess {

class Engine {
public:
    struct GoOptions {
        std::optional<std::vector<Move>> searchmoves{std::nullopt};
        bool ponder{false};
        std::optional<int64_t> wtime{std::nullopt};
        std::optional<int64_t> btime{std::nullopt};
        std::optional<int64_t> winc{std::nullopt};
        std::optional<int64_t> binc{std::nullopt};
        std::optional<int> movestogo{std::nullopt};
        std::optional<int> depth{std::nullopt};
        std::optional<int64_t> nodes{std::nullopt};
        std::optional<int64_t> movetime{std::nullopt};
        bool infinite{false};
    };

    Engine();
    ~Engine();

    void uci();
    void ucinewgame();
    void go(const GoOptions& opts);
    void stop();
    void ponderhit();
    void debug(bool on);
    bool debug() const;
    void set_option(const std::string& name, const uci::RawOptionValue& value);
    void isready() const;

    const Board& position() const;
    void set_position(Board new_board);
    void play_move(Move move);

private:
    struct CaseInsensitiveLess {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const {
            return std::lexicographical_compare(
                a.begin(), a.end(), b.begin(), b.end(),
                [](unsigned char x, unsigned char y) { return std::tolower(x) < std::tolower(y); });
        }
    };

    void handle_hash_opt();
    void handle_ponder_opt();
    void handle_multipv_opt();
    std::optional<std::chrono::steady_clock::time_point> deadline_from_go_opts(const GoOptions& opts) const;
    void search_report(const Search::SearchResult& result, std::chrono::milliseconds elapsed, int hashfull) const;
    void wait_for_search();

    Board board;

    int num_pvs{1};
    bool debug_on{false};
    std::map<std::string, uci::Option, CaseInsensitiveLess> options;

    Search search;
    std::vector<ZobristHash> hash_history;
    std::jthread search_thread{};
};

} // bears_chess