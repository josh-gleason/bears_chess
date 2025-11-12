#pragma once
#include <map>
#include "board.hpp"
#include "uci_options.hpp"

namespace bears_chess {

class Engine {
public:
    struct GoOptions {
        std::optional<std::vector<Move>> searchmoves{std::nullopt};
        bool ponder{false};
        std::optional<int> wtime{std::nullopt};
        std::optional<int> btime{std::nullopt};
        std::optional<int> winc{std::nullopt};
        std::optional<int> binc{std::nullopt};
        std::optional<int> movestogo{std::nullopt};
        std::optional<int> depth{std::nullopt};
        std::optional<int> nodes{std::nullopt};
        std::optional<int> movetime{std::nullopt};
        bool infinite{false};
    };

    Engine();

    void uci();
    void ucinewgame();
    void go(const GoOptions& opts);
    void stop();
    void ponderhit();
    void debug(bool on);
    bool debug() const;
    void set_option(const std::string& name, const uci::RawOptionValue& value);
    void isready() const;

    Board board;
    
    // TODO: transposition table

private:
    bool debug_on{false};
    std::map<std::string, uci::Option> options;

    void handle_hash_opt();
    void handle_ponder_opt();
    void handle_multipv_opt();
};

} // bears_chess