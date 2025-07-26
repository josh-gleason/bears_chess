#pragma once
#include "board.hpp"

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

    Engine() {}

    void ucinewgame() {
        // TODO
    }

    void go(const GoOptions& opts) {
        // TODO
    }

    void stop() {
        // TODO
    }

    void ponderhit() {
        // TODO
    }

    Board board;
};

} // bears_chess