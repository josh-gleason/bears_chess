#include "log.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <iostream>

namespace bears_chess {
namespace log {

constexpr auto DIAG_PATTERN = "[%H:%M:%S.%e %z] [thread %t] [%^%l%$] %v";

std::shared_ptr<spdlog::logger> logger_diag;
std::shared_ptr<spdlog::logger> logger_uci;

static spdlog::level::level_enum to_spd(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Debug: return spdlog::level::debug;
        case LogLevel::Info:  return spdlog::level::info;
        case LogLevel::Warn:  return spdlog::level::warn;
        case LogLevel::Error: return spdlog::level::err;
        case LogLevel::Fatal: return spdlog::level::critical;
        case LogLevel::Off:   return spdlog::level::off;
    }
    return spdlog::level::info;
}

void init(LogLevel level) {
    auto err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    err_sink->set_pattern(DIAG_PATTERN);

    auto out_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // simple sink without newline character
    out_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        "%v", spdlog::pattern_time_type::local, ""
    ));

    logger_diag = std::make_shared<spdlog::logger>("bears_diag", err_sink);
    logger_uci = std::make_shared<spdlog::logger>("bears_uci", out_sink);

    logger_diag->set_level(to_spd(level));
    logger_diag->flush_on(spdlog::level::info);

    // UCI/stdout should always print what you send it
    logger_uci->set_level(spdlog::level::trace);
    logger_uci->flush_on(spdlog::level::info);
}

void set_log_level(LogLevel level) { logger_diag->set_level(to_spd(level)); }

namespace detail {

// implied newline at end of message
void debug_sv(std::string_view v) { logger_diag->debug("{}", v); }
void info_sv(std::string_view v) { logger_diag->info("{}", v); }
void warn_sv(std::string_view v) { logger_diag->warn("{}", v); }
void error_sv(std::string_view v) { logger_diag->error("{}", v); }
void fatal_sv(std::string_view v) { logger_diag->critical("{}", v); }

void print_sv(std::string_view v) { logger_uci->info("{}", v); }
void println_sv(std::string_view v) { logger_uci->info("{}\n", v); }
void uci_print_sv(std::string_view v) { logger_uci->info("{}", v); }
void uci_println_sv(std::string_view v) { logger_uci->info("{}\n", v); }

}   // namespace detail
}   // namespace log
}   // namespace bears_chess
