#pragma once

#define ENABLE_BITMASK_OPERATORS(T) \
    constexpr T operator|(T a, T b) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) | static_cast<std::underlying_type_t<T>>(b)); } \
    constexpr T operator&(T a, T b) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) & static_cast<std::underlying_type_t<T>>(b)); } \
    constexpr T operator^(T a, T b) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ^ static_cast<std::underlying_type_t<T>>(b)); } \
    constexpr T& operator|=(T& a, T b) { return a = a | b; } \
    constexpr T& operator&=(T& a, T b) { return a = a & b; } \
    constexpr T& operator^=(T& a, T b) { return a = a ^ b; } \
    constexpr T operator~(T a) { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a)); }

#define ENABLE_PREINCREMENT(T) \
    constexpr T& operator++(T& a) { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + 1); } \
    constexpr T& operator--(T& a) { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - 1); }

#define ENABLE_SUM(T) \
    constexpr T operator+(T a, T b) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + static_cast<std::underlying_type_t<T>>(b)); } \
    constexpr T& operator+=(T& a, T b) { return a = a + b; } \
    constexpr T operator-(T a, T b) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - static_cast<std::underlying_type_t<T>>(b)); } \
    constexpr T& operator-=(T& a, T b) { return a = a - b; }

#define ENABLE_INEQUALITY(T) \
    constexpr bool operator<(T a, T b) { return static_cast<std::underlying_type_t<T>>(a) < static_cast<std::underlying_type_t<T>>(b); } \
    constexpr bool operator<=(T a, T b) { return static_cast<std::underlying_type_t<T>>(a) <= static_cast<std::underlying_type_t<T>>(b); } \
    constexpr bool operator>(T a, T b) { return static_cast<std::underlying_type_t<T>>(a) > static_cast<std::underlying_type_t<T>>(b); } \
    constexpr bool operator>=(T a, T b) { return static_cast<std::underlying_type_t<T>>(a) >= static_cast<std::underlying_type_t<T>>(b); }

#define ENABLE_FLAGS(T) \
    constexpr bool check_flag(T x, T f) { return static_cast<bool>(static_cast<std::underlying_type_t<T>>(x) & static_cast<std::underlying_type_t<T>>(f)); } \
    constexpr T set_flag(T x, T f) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) | static_cast<std::underlying_type_t<T>>(f)); } \
    constexpr T clear_flag(T x, T f) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) & (~static_cast<std::underlying_type_t<T>>(f))); } \
    constexpr T toggle_flag(T x, T f) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) ^ static_cast<std::underlying_type_t<T>>(f)); }
