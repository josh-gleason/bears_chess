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
#include "engine.hpp"

namespace bears_chess {

class CLI {
public:
    CLI();
    ~CLI();

    int run();
    
private:
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

    struct Command {
        CommandType type;
        std::string original;
        WordList args;
    };

    using CommandHandler = std::function<void(const Command&)>;

    const std::unordered_map<std::string, CommandType> command_types;
    const std::unordered_map<CommandType, CommandHandler> command_handlers;

    std::queue<Command> command_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;

    std::atomic<bool> exit_requested{false};
    std::thread input_thread;
    std::thread processor_thread;

    Command parse_command(const std::string& line) const;

    Engine engine;

    void input_listener();
    void command_processor();
    void enqueue_command(const Command& cmd);
    std::optional<Command> dequeue_command();

    // command handlers
    void handle_quit(const Command& cmd);
    void handle_display(const Command& cmd);
    void handle_perft(const Command& cmd);
    void handle_position(const Command& cmd);
    void handle_go(const Command& cmd);
    void handle_help(const Command& cmd);
    void handle_empty(const Command& cmd);
    void handle_unknown(const Command& cmd);
};

}