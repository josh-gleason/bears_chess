#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace bears_chess {

template<typename E> using hash_func_t = size_t(*)(const E&) noexcept;

// opt in traits for enum classes
struct bitwise_ops {};
struct shift_ops {};
struct arithmetic_ops {};
struct preincrement_ops {};
struct inequality_ops {};
struct flag_ops {};
struct boolean_ops {};
template<auto First, auto Last>
struct range_ops {
    static_assert(std::same_as<decltype(First), decltype(Last)>, "range_op<> parameters must be same enum type");
    static_assert(std::is_enum_v<decltype(First)>, "range_op<> parameters must be enum values");
    static constexpr decltype(First) first = First;
    static constexpr decltype(Last) last = Last;
};

template<typename E, hash_func_t<E> HashFunc, size_t HashMax>
struct hash_ops {
    using hash_input_type = E;
    static constexpr auto hash_func = HashFunc;
    static constexpr int hash_max = HashMax;
};

// default to no traits, opt in by specializing and extending enum_traits
// e.g. template<> struct enum_traits<Foo> : bitwise_op, increment_op {};
// to give Foo operator overloads for bitwise and increment ops
template<typename E>
struct enum_traits {};

template<typename E> concept BitmaskEnum = std::is_enum_v<E> && std::is_base_of_v<bitwise_ops, enum_traits<E>>;
template<BitmaskEnum T> constexpr T operator|(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) | static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T operator&(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) & static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T operator^(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ^ static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T operator%(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) % static_cast<std::underlying_type_t<T>>(b)); }
template<BitmaskEnum T> constexpr T& operator|=(T& a, T b) noexcept { return a = a | b; }
template<BitmaskEnum T> constexpr T& operator&=(T& a, T b) noexcept { return a = a & b; }
template<BitmaskEnum T> constexpr T& operator^=(T& a, T b) noexcept { return a = a ^ b; }
template<BitmaskEnum T> constexpr T& operator%=(T& a, T b) noexcept { return a = a % b; }
template<BitmaskEnum T> constexpr T operator~(T a) noexcept { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a)); }
template<BitmaskEnum T> constexpr T mask_if(bool c) { return static_cast<T>(-static_cast<std::make_signed_t<std::underlying_type_t<T>>>(c)); }

template<typename E> concept ShiftEnum = std::is_enum_v<E> && std::is_base_of_v<shift_ops, enum_traits<E>>;
template<ShiftEnum T> constexpr T operator<<(T a, int b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) << b); }
template<ShiftEnum T> constexpr T operator>>(T a, int b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) >> b); }
template<ShiftEnum T> constexpr T& operator<<=(T& a, int b) noexcept { return a = a << b; }
template<ShiftEnum T> constexpr T& operator>>=(T& a, int b) noexcept { return a = a >> b; }

template<typename E> concept ArithmeticEnum = std::is_enum_v<E> && std::is_base_of_v<arithmetic_ops, enum_traits<E>>;
template<ArithmeticEnum T> constexpr T operator+(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + static_cast<std::underlying_type_t<T>>(b)); }
template<ArithmeticEnum T> constexpr T operator-(T a, T b) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - static_cast<std::underlying_type_t<T>>(b)); }
template<ArithmeticEnum T> constexpr T operator-(T a) noexcept { return static_cast<T>(-static_cast<std::underlying_type_t<T>>(a)); }
template<ArithmeticEnum T> constexpr T& operator+=(T& a, T b) noexcept { return a = a + b; }
template<ArithmeticEnum T> constexpr T& operator-=(T& a, T b) noexcept { return a = a - b; }

template<typename E> concept PreincrementEnum = std::is_enum_v<E> && std::is_base_of_v<preincrement_ops, enum_traits<E>>;
template<PreincrementEnum T> constexpr T& operator++(T& a) noexcept { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) + 1); }
template<PreincrementEnum T> constexpr T& operator--(T& a) noexcept { return a = static_cast<T>(static_cast<std::underlying_type_t<T>>(a) - 1); }

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

template<typename E> concept BooleanEnum = std::is_enum_v<E> && std::is_base_of_v<boolean_ops, enum_traits<E>>;
template<BooleanEnum T> constexpr bool nonzero(T a) noexcept { return static_cast<std::underlying_type_t<T>>(a) != 0; }
template<BooleanEnum T> constexpr bool zero(T a) noexcept { return static_cast<std::underlying_type_t<T>>(a) == 0; }
template<BooleanEnum T> constexpr bool operator!(T a) noexcept { return static_cast<std::underlying_type_t<T>>(a) == 0; }

template<typename E> concept RangeEnum = std::is_enum_v<E> && std::is_base_of_v<range_ops<enum_traits<E>::first, enum_traits<E>::last>, enum_traits<E>> && requires {
    { enum_traits<E>::first } -> std::convertible_to<E>;
    { enum_traits<E>::last  } -> std::convertible_to<E>;
};
template<RangeEnum E> constexpr size_t num_of = static_cast<std::underlying_type_t<E>>(enum_traits<E>::last) - static_cast<std::underlying_type_t<E>>(enum_traits<E>::first) + 1;
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

template<typename E> concept HashEnum = std::is_enum_v<E> && std::is_base_of_v<hash_ops<typename enum_traits<E>::hash_input_type, enum_traits<E>::hash_func, enum_traits<E>::hash_max>, enum_traits<E>>;
template<HashEnum E> constexpr size_t hash(const E& in) { return enum_traits<E>::hash_func(in); }
template<HashEnum E> constexpr size_t hash_max = enum_traits<E>::hash_max;

} // namespace bears_chess
