#include "cli.hpp"
#include "bears_chess.hpp"

#include <print>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

namespace bears_chess {

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

std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    return tokens;
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
        { "q", CommandType::QUIT },
        { "quit", CommandType::QUIT },
        { "exit", CommandType::QUIT },
        { "d", CommandType::DISPLAY },
        { "display", CommandType::DISPLAY },
        { "perft", CommandType::PERFT },
        { "position", CommandType::POSITION },
        { "go", CommandType::GO },
        { "help", CommandType::HELP }
    },
    command_handlers{
        { CommandType::QUIT, [this] (const Command& cmd) { this->handle_quit(cmd); } },
        { CommandType::DISPLAY, [this] (const Command& cmd) { this->handle_display(cmd); } },
        { CommandType::PERFT, [this] (const Command& cmd) { this->handle_perft(cmd); } },
        { CommandType::POSITION, [this] (const Command& cmd) { this->handle_position(cmd); } },
        { CommandType::GO, [this] (const Command& cmd) { this->handle_go(cmd); } },
        { CommandType::HELP, [this] (const Command& cmd) { this->handle_help(cmd); } },
        { CommandType::EMPTY, [this] (const Command& cmd) { this->handle_empty(cmd); } },
        { CommandType::UNKNOWN, [this] (const Command& cmd) { this->handle_unknown(cmd); } }
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

void CLI::handle_quit(const Command& cmd) {
    if (!cmd.args.empty())
        throw std::invalid_argument(std::format("Unknown option {}", cmd.args[0]));
    exit_requested = true;
}

void CLI::handle_display(const Command& cmd) {
    if (!cmd.args.empty())
        throw std::invalid_argument(std::format("Unknown option {}", cmd.args[0]));
    std::println("{}", engine.board);
}

void CLI::handle_perft(const Command& cmd) {
    auto [max_depth, extras] = parse_args<int>(cmd.args);
    bool show_moves = false;
    bool show_stats = false;
    for (const auto& word : extras) {
        if (word == "moves" && !show_moves) {
            show_moves = true;
        } else if (word == "stats" && !show_stats) {
            show_stats = true;
        } else {
            throw std::invalid_argument(std::format("Unknown option {}", word));
        }
    }

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

void CLI::handle_position(const Command& cmd) {
    auto [arg1, extras] = parse_args<std::string>(cmd.args);
    size_t idx = 0;
    if (arg1 == "startpos") {
        engine.board = Board();
    } else if (arg1 == "fen") {
        std::istringstream sin(cmd.original);
        std::string fen;
        while (idx < extras.size() && extras[idx] != "moves") {
            if (!fen.empty()) {
                fen += " ";
            }
            fen += extras[idx++];
        }
        // TODO: verify fen string
        engine.board = load_fen(fen);
    } else {
        throw std::invalid_argument(std::format("Unknown option {}", arg1));
    }

    if (idx < extras.size()) {
        if (extras[idx] != "moves") {
            throw std::invalid_argument(std::format("Unknown option {}", extras[idx]));
        }

        idx++;
        while (idx < extras.size()) {
            engine.board.do_move(parse_uci_move(extras[idx], engine.board));
            idx++;
        }
    }
}

void CLI::handle_go(const Command& cmd) {
    // TODO
    std::println("COMMAND: go");
}

void CLI::handle_help(const Command& cmd) {
    // TODO
    std::println("COMMAND: help");
}

void CLI::handle_empty(const Command& cmd) {
    std::println("COMMAND: empty");
}

void CLI::handle_unknown(const Command& cmd) {
    throw std::invalid_argument(std::format("Unknown command: {}", cmd.original));
}

} // bears_chess
