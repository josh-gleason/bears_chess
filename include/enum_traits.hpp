#pragma once
#include <type_traits>

// opt in traits for enum classes
struct bitmask_ops {};
struct preincrement_ops {};
struct sum_ops {};
struct inequality_ops {};
struct flag_ops {};
template<auto First, auto Last>
struct range_ops {
    static_assert(std::same_as<decltype(First), decltype(Last)>, "range_op<> parameters must be same enum type");
    static_assert(std::is_enum_v<decltype(First)>, "range_op<> parameters must be enum values");
    static constexpr decltype(First) first = First;
    static constexpr decltype(Last) last = Last;
};

// default to no traits, opt in by specializing and extending enum_traits
// e.g. template<> struct enum_traits<Foo> : bitwise_op, increment_op {};
// to give Foo operator overloads for bitwise and increment ops
template<typename E>
struct enum_traits {};

template<typename E> concept BitmaskEnum = std::is_enum_v<E> && std::is_base_of_v<bitmask_ops, enum_traits<E>>;
template<BitmaskEnum T> constexpr T operator|(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) | static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T operator&(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) & static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T operator^(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ^ static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T& operator|=(T& a, T b) noexcept { return a = a | b; }
template<BitmaskEnum T> constexpr T& operator&=(T& a, T b) noexcept { return a = a & b; }
template<BitmaskEnum T> constexpr T& operator^=(T& a, T b) noexcept { return a = a ^ b; }
template<BitmaskEnum T> constexpr T operator~(T a) noexcept { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a)); }

template<typename E> concept PreincrementEnum = std::is_enum_v<E> && std::is_base_of_v<preincrement_ops, enum_traits<E>>;
template<PreincrementEnum T> constexpr T& operator++(T& a) noexcept { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + 1); }
template<PreincrementEnum T> constexpr T& operator--(T& a) noexcept { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - 1); }

template<typename E> concept SumEnum = std::is_enum_v<E> && std::is_base_of_v<sum_ops, enum_traits<E>>;
template<SumEnum T> constexpr T operator+(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + static_cast<std::underlying_type_t<T>>(b)); }
template<SumEnum T> constexpr T& operator+=(T& a, T b) noexcept { return a = a + b; }
template<SumEnum T> constexpr T operator-(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - static_cast<std::underlying_type_t<T>>(b)); }
template<SumEnum T> constexpr T& operator-=(T& a, T b) noexcept { return a = a - b; }

template<typename E> concept InequalityEnum = std::is_enum_v<E> && std::is_base_of_v<inequality_ops, enum_traits<E>>;
template<InequalityEnum T> constexpr bool operator<(T a, T b) noexcept { return static_cast<std::underlying_type_t<T>>(a) < static_cast<std::underlying_type_t<T>>(b); }
template<InequalityEnum T> constexpr bool operator<=(T a, T b) noexcept { return static_cast<std::underlying_type_t<T>>(a) <= static_cast<std::underlying_type_t<T>>(b); }
template<InequalityEnum T> constexpr bool operator>(T a, T b) noexcept { return static_cast<std::underlying_type_t<T>>(a) > static_cast<std::underlying_type_t<T>>(b); }
template<InequalityEnum T> constexpr bool operator>=(T a, T b) noexcept { return static_cast<std::underlying_type_t<T>>(a) >= static_cast<std::underlying_type_t<T>>(b); }

template<typename E> concept FlagEnum = std::is_enum_v<E> && std::is_base_of_v<flag_ops, enum_traits<E>>;
template<FlagEnum T> constexpr bool check_flag(T x, T f) noexcept { return static_cast<bool>(static_cast<std::underlying_type_t<T>>(x) & static_cast<std::underlying_type_t<T>>(f)); }
template<FlagEnum T> constexpr T set_flag(T x, T f) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) | static_cast<std::underlying_type_t<T>>(f)); }
template<FlagEnum T> constexpr T clear_flag(T x, T f) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) & (~static_cast<std::underlying_type_t<T>>(f))); }
template<FlagEnum T> constexpr T toggle_flag(T x, T f) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(x) ^ static_cast<std::underlying_type_t<T>>(f)); }

template<typename E> concept RangeEnum = requires {
    { enum_traits<E>::first } -> std::convertible_to<E>;
    { enum_traits<E>::last  } -> std::convertible_to<E>;
};
template<RangeEnum E> constexpr size_t num_of = []() { return (static_cast<std::underlying_type_t<E>>(enum_traits<E>::last) - static_cast<std::underlying_type_t<E>>(enum_traits<E>::first) + 1); }();
template<RangeEnum E> constexpr E first = enum_traits<E>::first;
template<RangeEnum E> constexpr E last = enum_traits<E>::last;
template<RangeEnum E> constexpr std::array<E, num_of<E>> iter = [](){
    std::array<E, num_of<E>> ary;
    for (size_t i = 0; i < num_of<E>; ++i)
        ary[i] = static_cast<E>(static_cast<std::underlying_type_t<E>>(enum_traits<E>::first) + i);
    return ary;
}();

template<RangeEnum E> constexpr std::array<E, num_of<E>> iter_rev = [](){
    std::array<E, num_of<E>> ary;
    for (size_t i = 0; i < num_of<E>; ++i)
        ary[i] = static_cast<E>(static_cast<std::underlying_type_t<E>>(enum_traits<E>::last) - i);
    return ary;
}();
