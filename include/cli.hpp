#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>
#include <optional>
#include <functional>
#include <unordered_map>
#include "async_line_reader.hpp"
#include "engine.hpp"

namespace bears_chess {

class CLI {
public:
    CLI();
    ~CLI();

    int run();
    
private:
    using RawCommand = std::string;
    using Word = std::string;
    using WordList = std::vector<Word>;

    enum class CommandType {
        QUIT,
        DISPLAY,
        PERFT,
        POSITION,
        GO,
        HELP,
        EMPTY,
        UNKNOWN
    };

    struct ParsedCommand {
        CommandType type;
        RawCommand original;
        WordList args;
    };

    using CommandHandler = std::function<void(const ParsedCommand&)>;

    const std::unordered_map<RawCommand, CommandType> command_types;
    const std::unordered_map<CommandType, CommandHandler> command_handlers;

    std::queue<RawCommand> command_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;

    std::atomic<bool> exit_requested{false};
    AsyncLineReader reader;
    std::thread processor_thread;

    ParsedCommand parse_command(const RawCommand& line) const;

    Engine engine;

    void input_listener();
    void command_processor();
    void enqueue_command(const RawCommand& cmd);
    std::optional<RawCommand> dequeue_command();

    // command handlers
    void handle_quit(const ParsedCommand& cmd);
    void handle_display(const ParsedCommand& cmd);
    void handle_perft(const ParsedCommand& cmd);
    void handle_position(const ParsedCommand& cmd);
    void handle_go(const ParsedCommand& cmd);
    void handle_help(const ParsedCommand& cmd);
    void handle_empty(const ParsedCommand& cmd);
    void handle_unknown(const ParsedCommand& cmd);
};

}