#include "cli.hpp"
#include "bears_chess.hpp"

#include <print>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

namespace bears_chess {

std::string to_lower(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(r), ::tolower);
    return r;
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
        { "quit", CommandType::QUIT },
        { "exit", CommandType::QUIT },
        { "d", CommandType::DISPLAY },
        { "display", CommandType::DISPLAY },
        { "perft", CommandType::PERFT },
        { "divide", CommandType::DIVIDE },
        { "position", CommandType::POSITION },
        { "go", CommandType::GO },
        { "help", CommandType::HELP }
    },
    command_handlers{
        { CommandType::QUIT, [this] (const ParsedCommand& cmd) { this->handle_quit(cmd); } },
        { CommandType::DISPLAY, [this] (const ParsedCommand& cmd) { this->handle_display(cmd); } },
        { CommandType::PERFT, [this] (const ParsedCommand& cmd) { this->handle_perft(cmd); } },
        { CommandType::DIVIDE, [this] (const ParsedCommand& cmd) { this->handle_divide(cmd); } },
        { CommandType::POSITION, [this] (const ParsedCommand& cmd) { this->handle_position(cmd); } },
        { CommandType::GO, [this] (const ParsedCommand& cmd) { this->handle_go(cmd); } },
        { CommandType::HELP, [this] (const ParsedCommand& cmd) { this->handle_help(cmd); } },
        { CommandType::EMPTY, [this] (const ParsedCommand& cmd) { this->handle_empty(cmd); } },
        { CommandType::UNKNOWN, [this] (const ParsedCommand& cmd) { this->handle_unknown(cmd); } }
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

    // threads exit when exit command provided
    input_thread.join();
    processor_thread.join();

    return 0;
}

void CLI::input_listener() {
    std::string line;
    while (!exit_requested && std::getline(std::cin, line)) {
        enqueue_command(RawCommand(line));
    }
}

CLI::ParsedCommand CLI::parse_command(const RawCommand& line) const {
    std::vector<std::string> words = split(to_lower(line));
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
        std::optional<RawCommand> cmd_opt = dequeue_command();
        if (!cmd_opt)
            continue;

        ParsedCommand cmd = parse_command(*cmd_opt);
        command_handlers.at(cmd.type)(cmd);
    }
}

void CLI::enqueue_command(const RawCommand& cmd) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    command_queue.push(cmd);
    queue_cv.notify_one();
}

std::optional<CLI::RawCommand> CLI::dequeue_command() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock, [this] {
        return exit_requested || !command_queue.empty();
    });

    if (!command_queue.empty()) {
        RawCommand cmd = std::move(command_queue.front());
        command_queue.pop();
        return cmd;
    }
    return std::nullopt;
}

// command handlers

void CLI::handle_quit(const ParsedCommand& cmd) {
    std::println("COMMAND: quit");
    exit_requested = true;
    std::cin.setstate(std::ios::eofbit);
}

void CLI::handle_display(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: display");
}

void CLI::handle_perft(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: perft");
}

void CLI::handle_divide(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: divide");
}

void CLI::handle_position(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: position");
}

void CLI::handle_go(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: go");
}

void CLI::handle_help(const ParsedCommand& cmd) {
    // TODO
    std::println("COMMAND: help");
}

void CLI::handle_empty(const ParsedCommand& cmd) {
    std::println("COMMAND: empty");
}

void CLI::handle_unknown(const ParsedCommand& cmd) {
    std::println("COMMAND: unknown");
    std::println("Unknown command: {}", cmd.original);
}

} // bears_chess
