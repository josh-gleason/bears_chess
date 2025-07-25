#include "async_line_reader.hpp"
#include <iostream>

namespace bears_chess {

AsyncLineReader::AsyncLineReader(LineHandler handler) :
    input(io_ctx, STDIN_FILENO),
    handler(std::move(handler))
{}

AsyncLineReader::~AsyncLineReader() {
    stop();
}

void AsyncLineReader::start() {
    if (running.exchange(true))
        return;

    do_read();
    thread = std::thread([this]{ io_ctx.run(); });
}

void AsyncLineReader::stop() {
    if (!running.exchange(false))
        return;

    io_ctx.stop();

    if (thread.joinable())
        thread.join();
}

void AsyncLineReader::do_read() {
    if (!running) return;
    asio::async_read_until(
        input, buffer, '\n',
        [this](auto&& ec, auto bytes){ on_read(ec, bytes); }
    );
}

void AsyncLineReader::on_read(const asio::error_code& ec, std::size_t) {
    if (!running || ec) {
        return;
    }

    std::istream is(&buffer);
    std::string line;
    std::getline(is, line);

    try {
        handler(std::move(line));
    } catch(...) {}

    do_read();
}

} // bears_chess
