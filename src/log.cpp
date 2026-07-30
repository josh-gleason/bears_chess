#include "log.hpp"
#include <ranges>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <iostream>

namespace bears_chess {
namespace log {

constexpr auto DIAG_PATTERN = "[%H:%M:%S.%e %z] [thread %t] [%^%l%$] %v";

std::shared_ptr<spdlog::logger> logger_diag;
std::shared_ptr<spdlog::logger> logger_human;
std::shared_ptr<spdlog::logger> logger_uci;
std::shared_ptr<spdlog::logger> logger_uci_info;


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
    // stderr diagnostic log sink
    auto diag_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    diag_sink->set_pattern(DIAG_PATTERN);
    logger_diag = std::make_shared<spdlog::logger>("bears_diag", diag_sink);
    logger_diag->set_level(to_spd(level));
    logger_diag->flush_on(spdlog::level::info);

    // stdout logger for general non-UCI messages, suppress these in UCI mode
    auto human_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    human_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        "%v", spdlog::pattern_time_type::local, ""
    ));
    logger_human = std::make_shared<spdlog::logger>("bears_human", human_sink);
    logger_human->set_level(spdlog::level::trace);
    logger_human->flush_on(spdlog::level::info);

    // stdout logger for UCI message
    auto uci_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    uci_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        "%v", spdlog::pattern_time_type::local, ""
    ));    
    logger_uci = std::make_shared<spdlog::logger>("bears_uci", uci_sink);
    logger_uci->set_level(spdlog::level::trace);
    logger_uci->flush_on(spdlog::level::info);

    // stdout logger for UCI info messages shown in UCI mode when debug option is set
    auto uci_info_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    uci_info_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
        "%v", spdlog::pattern_time_type::local, ""
    ));
    logger_uci_info = std::make_shared<spdlog::logger>("bears_uci_info", uci_info_sink);
    logger_uci_info->set_level(spdlog::level::off);
    logger_uci_info->flush_on(spdlog::level::info);
}

void set_log_level(LogLevel level) {
    logger_diag->set_level(to_spd(level));
}

void set_uci_mode(bool on) {
    logger_human->set_level(on ? spdlog::level::off : spdlog::level::trace);
    if (!on) {
        logger_uci_info->set_level(spdlog::level::off);
    }
}

void set_uci_debug(bool on) {
    logger_uci_info->set_level(on ? spdlog::level::trace : spdlog::level::off);
}

namespace detail {

// implied newline at end of message
void debug_sv(std::string_view v) { logger_diag->debug("{}", v); }
void info_sv(std::string_view v) { logger_diag->info("{}", v); }
void warn_sv(std::string_view v) { logger_diag->warn("{}", v); }
void error_sv(std::string_view v) { logger_diag->error("{}", v); }
void fatal_sv(std::string_view v) { logger_diag->critical("{}", v); }

void print_sv(std::string_view v) { logger_human->info("{}", v); }
void println_sv(std::string_view v) { logger_human->info("{}\n", v); }
void uci_print_sv(std::string_view v) { logger_uci->info("{}", v); }
void uci_println_sv(std::string_view v) { logger_uci->info("{}\n", v); }

void uci_info_sv(std::string_view v) {
    if (!logger_uci_info->should_log(spdlog::level::info)) {
        return;
    }

    std::string out;
    out.reserve(v.size());
    for (const auto& chunk : std::views::split(v, '\n')) {
        std::string_view line(chunk);
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        // drop blank lines
        if (line.find_first_not_of(" \t") == std::string_view::npos) {
            continue;
        }
        out += "info string ";
        out += line;
        out += '\n';
    }

    if (!out.empty()) {
        logger_uci_info->info("{}", out);
    }
}

}   // namespace detail
}   // namespace log
}   // namespace bears_chess
