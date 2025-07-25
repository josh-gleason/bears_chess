#pragma once
#include <asio.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <thread>
#include <functional>
#include <atomic>

namespace bears_chess {

class AsyncLineReader {
public:
    using LineHandler = std::function<void(std::string)>;

    explicit AsyncLineReader(LineHandler handler);
    ~AsyncLineReader();

    void start();
    void stop();
private:
    void do_read();
    void on_read(const asio::error_code& ec, std::size_t bytes);

    asio::io_context io_ctx;
    asio::posix::stream_descriptor input;
    asio::streambuf buffer;
    LineHandler handler;
    std::thread thread;
    std::atomic<bool> running{false};
};

} // namespace bears_chess
