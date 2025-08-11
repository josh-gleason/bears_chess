#pragma once
#include <string_view>
#include <source_location>
#include <format>
#include <utility>

namespace bears_chess {
namespace log {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off,
};

namespace detail {
void debug_sv(std::string_view v);
void info_sv(std::string_view v);
void warn_sv(std::string_view v);
void error_sv(std::string_view v);
void fatal_sv(std::string_view v);

// print to stdout but only if not in UCI mode
void print_sv(std::string_view v);
void println_sv(std::string_view v);

// print directly to stdout even in UCI mode
void uci_print_sv(std::string_view v);
void uci_println_sv(std::string_view v);
}

void init(LogLevel level = LogLevel::Info);
void set_log_level(LogLevel level);

// too much copy-paste but not sure of a better way without ugly macros
template<class... Args>
inline void debug(std::format_string<Args...> fmt, Args&&... args) {
    detail::debug_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void debug(std::string_view s) { detail::debug_sv(s); }

template<class... Args>
inline void info(std::format_string<Args...> fmt, Args&&... args) {
    detail::info_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void info(std::string_view s) { detail::info_sv(s); }

template<class... Args>
inline void warn(std::format_string<Args...> fmt, Args&&... args) {
    detail::warn_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void warn(std::string_view s) { detail::warn_sv(s); }

template<class... Args>
inline void error(std::format_string<Args...> fmt, Args&&... args) {
    detail::error_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void error(std::string_view s) { detail::error_sv(s); }

template<class... Args>
inline void fatal(std::format_string<Args...> fmt, Args&&... args) {
    detail::fatal_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void fatal(std::string_view s) { detail::fatal_sv(s); }

template<class... Args>
inline void print(std::format_string<Args...> fmt, Args&&... args) {
    detail::print_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void print(std::string_view s) { detail::print_sv(s); }

template<class... Args>
inline void println(std::format_string<Args...> fmt, Args&&... args) {
    detail::println_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void println(std::string_view s) { detail::println_sv(s); }

template<class... Args>
inline void uci_print(std::format_string<Args...> fmt, Args&&... args) {
    detail::uci_print_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void uci_print(std::string_view s) { detail::uci_print_sv(s); }

template<class... Args>
inline void uci_println(std::format_string<Args...> fmt, Args&&... args) {
    detail::uci_println_sv(std::format(fmt, std::forward<Args>(args)...));
}
inline void uci_println(std::string_view s) { detail::uci_println_sv(s); }

}   // namespace log
}   // namespace bears_chess

