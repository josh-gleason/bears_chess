#include "cli.hpp"
#include "bears_chess.hpp"
#include "parse_utils.hpp"
#include "help/messages.hpp"

#include <vector>

namespace bears_chess {

using log::print, log::println;

CLI::CLI() :
    command_types{
        // Non-UCI commands
        { "display", CommandType::DISPLAY },
        { "d", CommandType::DISPLAY },
        { "perft", CommandType::PERFT },
        { "help", CommandType::HELP },
        { "q", CommandType::QUIT },
        { "exit", CommandType::QUIT },
        // UCI commands
        { "uci", CommandType::UCI },
        { "debug", CommandType::DEBUG },
        { "isready", CommandType::ISREADY },
        { "setoption", CommandType::SETOPTION },
        { "register", CommandType::REGISTER },
        { "ucinewgame", CommandType::UCINEWGAME },
        { "position", CommandType::POSITION },
        { "go", CommandType::GO },
        { "stop", CommandType::STOP },
        { "ponderhit", CommandType::PONDERHIT },
        { "quit", CommandType::QUIT },
    },
    command_handlers{
        { CommandType::EMPTY, [this] (const Command& cmd) { this->handle_empty(cmd); } },
        { CommandType::UNKNOWN, [this] (const Command& cmd) { this->handle_unknown(cmd); } },
        { CommandType::DISPLAY, [this] (const Command& cmd) { this->handle_display(cmd); } },
        { CommandType::PERFT, [this] (const Command& cmd) { this->handle_perft(cmd); } },
        { CommandType::HELP, [this] (const Command& cmd) { this->handle_help(cmd); } },
        { CommandType::UCI, [this] (const Command& cmd) { this->handle_uci(cmd); } },
        { CommandType::DEBUG, [this] (const Command& cmd) { this->handle_debug(cmd); } },
        { CommandType::ISREADY, [this] (const Command& cmd) { this->handle_isready(cmd); } },
        { CommandType::SETOPTION, [this] (const Command& cmd) { this->handle_setoption(cmd); } },
        { CommandType::REGISTER, [this] (const Command& cmd) { this->handle_register(cmd); } },
        { CommandType::UCINEWGAME, [this] (const Command& cmd) { this->handle_ucinewgame(cmd); } },
        { CommandType::POSITION, [this] (const Command& cmd) { this->handle_position(cmd); } },
        { CommandType::GO, [this] (const Command& cmd) { this->handle_go(cmd); } },
        { CommandType::STOP, [this] (const Command& cmd) { this->handle_stop(cmd); } },
        { CommandType::PONDERHIT, [this] (const Command& cmd) { this->handle_ponderhit(cmd); } },
        { CommandType::QUIT, [this] (const Command& cmd) { this->handle_quit(cmd); } },
    },
    help_messages{
        { CommandType::EMPTY, HELP },
        { CommandType::UNKNOWN, HELP_UNKNOWN },
        { CommandType::DISPLAY, HELP_DISPLAY },
        { CommandType::PERFT, HELP_PERFT },
        { CommandType::HELP, HELP_HELP },
        { CommandType::UCI, HELP_UCI },
        { CommandType::DEBUG, HELP_DEBUG },
        { CommandType::ISREADY, HELP_ISREADY },
        { CommandType::SETOPTION, HELP_SETOPTION },
        { CommandType::REGISTER, HELP_REGISTER },
        { CommandType::UCINEWGAME, HELP_UCINEWGAME },
        { CommandType::POSITION, HELP_POSITION },
        { CommandType::GO, HELP_GO },
        { CommandType::STOP, HELP_STOP },
        { CommandType::PONDERHIT, HELP_PONDERHIT },
        { CommandType::QUIT, HELP_QUIT },
    }
{}

CLI::~CLI() {
    exit_requested = true;

    if (input_thread.joinable())
        input_thread.join();
    if (processor_thread.joinable())
        processor_thread.join();
}

int CLI::run() {
    println("Welcome to the Bear's Chess Engine");

    input_thread = std::thread(&CLI::input_listener, this);
    processor_thread = std::thread(&CLI::command_processor, this);

    processor_thread.join();
    return 0;
}

void CLI::input_listener() {
    std::string line;
    CommandType cmd_type = CommandType::EMPTY;
    while (cmd_type != CommandType::QUIT && !exit_requested && std::getline(std::cin, line)) {
        Command cmd = parse_command(line);
        cmd_type = cmd.type;
        enqueue_command(std::move(cmd));
    }
}

CLI::Command CLI::parse_command(const std::string& line) const {
    std::vector<std::string> words = split(line);
    if (words.empty()) {
        return { CommandType::EMPTY, line, {} };
    }
    const std::string& cmd = words[0];
    CommandType type = get_or_default(command_types, cmd, CommandType::UNKNOWN);
    WordList args(words.begin() + 1, words.end());
    return { type, line, std::move(args) };
}

void CLI::command_processor() {
    while (!exit_requested) {
        std::optional<Command> cmd_opt = dequeue_command();
        if (!cmd_opt)
            continue;

        try {
            command_handlers.at(cmd_opt->type)(*cmd_opt);
        } catch (std::invalid_argument err) {
            log::error("{}", err.what());
        }
    }
}

void CLI::enqueue_command(const Command& cmd) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    command_queue.push(cmd);
    queue_cv.notify_one();
}

std::optional<CLI::Command> CLI::dequeue_command() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock, [this] {
        return exit_requested || !command_queue.empty();
    });

    if (!command_queue.empty()) {
        Command cmd = std::move(command_queue.front());
        command_queue.pop();
        return cmd;
    }
    return std::nullopt;
}

// command handlers

void CLI::handle_empty(const Command& cmd) {}

void CLI::handle_unknown(const Command& cmd) {
    throw std::invalid_argument(std::format("Unknown command: {}", cmd.original));
}

void CLI::handle_display(const Command& cmd) {
    println("{}", engine.position());
}

void CLI::handle_perft(const Command& cmd) {
    auto [max_depth, flags] = parse_args<int>(cmd.args);
    bool show_moves = contains(flags, "moves");
    bool show_stats = contains(flags, "stats");

    if (!show_stats && !show_moves) {
        println("{}", run_perft<LegalPolicy, false, false>(engine.position(), max_depth));
    } else if (!show_stats && show_moves) {
        println("{}", run_perft<LegalPolicy, false, true>(engine.position(), max_depth));
    } else if (show_stats && !show_moves) {
        println("{}", run_perft<LegalPolicy, true, false>(engine.position(), max_depth));
    } else {
        println("{}", run_perft<LegalPolicy, true, true>(engine.position(), max_depth));
    }
}

void CLI::handle_help(const Command& cmd) {
    if (cmd.args.empty()) {
        println("{}", help_messages.find(CommandType::EMPTY)->second);
    } else {
        std::string cmd_str = *cmd.args.begin();
        CommandType cmd_type = get_or_default(command_types, cmd_str, CommandType::UNKNOWN);
        println("{}", help_messages.find(cmd_type)->second);
    }
}

void CLI::handle_uci(const Command& cmd) {
    engine.uci();
}

void CLI::handle_debug(const Command& cmd) {
    engine.debug(contains(cmd.args, "on"));
}

void CLI::handle_isready(const Command& cmd) {
    engine.isready();
}

void CLI::handle_setoption(const Command& cmd) {
    auto grouped = group_by_keywords(cmd.args, {"name", "value"});
    if (!grouped.contains("name")) {
        throw std::invalid_argument("Missing name field");
    }

    // UCI names can be multiple words
    std::string name = join(grouped["name"], " ");

    uci::RawOptionValue value{};
    if (grouped.contains("value")) {
        value = std::move(grouped["value"]);
    }

    engine.set_option(name, value);
}

void CLI::handle_register(const Command& cmd) {
    // TODO
    auto grouped = group_by_keywords(cmd.args, {"later", "name", "code"});
    log::error("COMMAND: register not implemented");
}

void CLI::handle_ucinewgame(const Command& cmd) {
    engine.ucinewgame();
}

void CLI::handle_position(const Command& cmd) {
    auto grouped = group_by_keywords(cmd.args, {"startpos", "fen", "moves"});
    if (grouped.contains("startpos")) {
        engine.set_position(Board());
    } else if (grouped.contains("fen")) {
        engine.set_position(load_fen(join(grouped["fen"], " ")));
    }

    if (grouped.contains("moves")) {
        for (auto& move_str : grouped["moves"]) {
            engine.play_move(convert_uci_to_move(move_str, engine.position()));
        }
    }
}

void CLI::handle_go(const Command& cmd) {
    auto grouped = group_by_keywords(cmd.args, {
        "searchmoves", "ponder", "wtime", "btime", "winc", "binc",
        "movestogo", "depth", "nodes", "mate", "movetime", "infinite"
    });

    Engine::GoOptions opts;

    if (grouped.contains("searchmoves")) {
        opts.searchmoves = std::vector<Move>(grouped["searchmoves"].size());
        auto it = opts.searchmoves->begin();
        for (auto& move_str : grouped["searchmoves"]) {
            *(it++) = convert_uci_to_move(move_str, engine.position());
        }
    }
    if (grouped.contains("ponder")) {
        opts.ponder = true;
    }
    if (grouped.contains("wtime")) {
        opts.wtime = std::get<0>(parse_args<int>(grouped["wtime"]));
    }
    if (grouped.contains("btime")) {
        opts.btime = std::get<0>(parse_args<int>(grouped["btime"]));
    }
    if (grouped.contains("winc")) {
        opts.winc = std::get<0>(parse_args<int>(grouped["winc"]));
    }
    if (grouped.contains("binc")) {
        opts.binc = std::get<0>(parse_args<int>(grouped["binc"]));
    }
    if (grouped.contains("movestogo")) {
        opts.movestogo = std::get<0>(parse_args<int>(grouped["movestogo"]));
    }
    if (grouped.contains("depth")) {
        opts.depth = std::get<0>(parse_args<int>(grouped["depth"]));
    }
    if (grouped.contains("nodes")) {
        opts.nodes = std::get<0>(parse_args<int>(grouped["nodes"]));
    }
    if (grouped.contains("movetime")) {
        opts.movetime = std::get<0>(parse_args<int>(grouped["movetime"]));
    }
    if (grouped.contains("infinite")) {
        opts.infinite = true;
    }

    engine.go(opts);
}

void CLI::handle_stop(const Command& cmd) {
    engine.stop();
}

void CLI::handle_ponderhit(const Command& cmd) {
    engine.ponderhit();
}

void CLI::handle_quit(const Command& cmd) {
    exit_requested = true;
}

} // bears_chess
