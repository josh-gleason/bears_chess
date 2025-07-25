#include <bears_chess.hpp>

using bears_chess::CLI, bears_chess::init;

int main(int argc, char* argv[]) {
    init();
    CLI cli;
    return cli.run();
}
