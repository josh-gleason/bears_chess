#include "bindings.hpp"

#include <bears_chess.hpp>

using namespace bears_chess_py;

NB_MODULE(bears_chess, m) {
    bears_chess::init();
    bind_move(m);
    bind_board(m);
    bind_search(m);
    bind_mcts(m);
    bind_attrs(m);
}