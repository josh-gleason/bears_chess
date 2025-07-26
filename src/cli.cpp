#include "cli.hpp"
#include "bears_chess.hpp"

#include <print>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

namespace bears_chess {

using KeyedArgs = std::unordered_map<std::string, std::vector<std::string>>;

template<typename> constexpr bool is_optional_impl = false;
template<typename T> constexpr bool is_optional_impl<std::optional<T>> = true;

template<typename T> 
constexpr bool is_optional = is_optional_impl<std::remove_cvref_t<T>>;

template<typename T>
T parse_scalar(const std::string& s)
{
    if constexpr (std::same_as<T, std::string>) {
        return s;
    } else {
        T v{};
        auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), v);
        if (err != std::errc() || ptr != s.data() + s.size())
            throw std::invalid_argument(std::format("Failed to parse: {}", s));
        return v;
    }
}

template<typename... Ts>
auto parse_args(const std::vector<std::string>& args) {
    std::size_t idx = 0;

    auto parse_one = [&](auto tag) {
        using U = typename decltype(tag)::type;

        if constexpr (is_optional<U>) {
            if (idx < args.size())
                return U{ parse_scalar<typename U::value_type>(args[idx++]) };
            else
                return U{};
        } else {
            // mandatory
            if (idx >= args.size())
                throw std::invalid_argument(std::format("Missing argument at position {}", idx));
            return parse_scalar<U>(args[idx++]);
        }
    };

    auto parsed_args = std::make_tuple(parse_one(std::type_identity<Ts>{})...);

    std::vector<std::string> extra_args;
    if (idx < args.size()) {
        extra_args.assign(args.begin() + idx, args.end());
    }

    return std::tuple_cat(parsed_args, std::tuple{ std::move(extra_args) });
}

KeyedArgs group_by_keywords(const std::vector<std::string>& args, const std::vector<std::string>& keywords) {
    KeyedArgs result;
    std::string current = "";
    result[current];

    for (auto& word : args) {
        if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
            current = word;
            result[current];
        } else {
            result[current].push_back(word);
        }
    }
    return result;
}

std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    return tokens;
}

std::string join(const std::vector<std::string>& words, const std::string& delimiter=" ") {
    if (words.empty()) {
        return "";
    }
    std::string result = words[0];
    for (auto it = words.cbegin() + 1; it != words.cend(); ++it) {
        result += delimiter + *it;
    }
    return result;
}

bool contains(const std::vector<std::string>& words, const std::string& value) {
    return std::find(words.begin(), words.end(), value) != words.end();
}

template<typename K, typename T>
T get_or_default(const std::unordered_map<K, T>& map, const K& key, const T& default_) {
    auto it = map.find(key);
    if (it == map.end()) {
        return default_;
    }
    return it->second;
}

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
    std::println("Welcome to the Bear's Chess Engine");

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
            std::println("ERROR: {}", err.what());
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
    std::println("{}", engine.board);
}

void CLI::handle_perft(const Command& cmd) {
    auto [max_depth, flags] = parse_args<int>(cmd.args);
    bool show_moves = contains(flags, "moves");
    bool show_stats = contains(flags, "stats");

    if (!show_stats && !show_moves) {
        std::println("{}", run_perft<LegalPolicy, false, false>(engine.board, max_depth));
    } else if (!show_stats && show_moves) {
        std::println("{}", run_perft<LegalPolicy, false, true>(engine.board, max_depth));
    } else if (show_stats && !show_moves) {
        std::println("{}", run_perft<LegalPolicy, true, false>(engine.board, max_depth));
    } else {
        std::println("{}", run_perft<LegalPolicy, true, true>(engine.board, max_depth));
    }
}

void CLI::handle_help(const Command& cmd) {
    // TODO
    std::println("COMMAND: help");
}

void CLI::handle_uci(const Command& cmd) {
    // TODO
    std::println("COMMAND: uci");
}

void CLI::handle_debug(const Command& cmd) {
    // TODO
    bool on = contains(cmd.args, "on");
    std::println("COMMAND: debug");
}

void CLI::handle_isready(const Command& cmd) {
    std::println("readyok");
}

void CLI::handle_setoption(const Command& cmd) {
    // TODO
    auto grouped = group_by_keywords(cmd.args, {"name", "value"});
    std::println("COMMAND: setoption");
}

void CLI::handle_register(const Command& cmd) {
    // TODO
    auto grouped = group_by_keywords(cmd.args, {"later", "name", "code"});
    std::println("COMMAND: register");
}

void CLI::handle_ucinewgame(const Command& cmd) {
    engine.ucinewgame();
}

void CLI::handle_position(const Command& cmd) {
    auto grouped = group_by_keywords(cmd.args, {"startpos", "fen", "moves"});
    if (grouped.contains("startpos")) {
        engine.board = Board();
    } else if (grouped.contains("fen")) {
        engine.board = load_fen(join(grouped["fen"], " "));
    }

    if (grouped.contains("moves")) {
        for (auto& move_str : grouped["moves"]) {
            engine.board.do_move(parse_uci_move(move_str, engine.board));
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
            *(it++) = parse_uci_move(move_str, engine.board);
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
