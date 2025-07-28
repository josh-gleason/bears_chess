#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <sstream>

namespace bears_chess {

template<typename> constexpr bool is_optional_impl = false;
template<typename T> constexpr bool is_optional_impl<std::optional<T>> = true;

template<typename T> 
constexpr bool is_optional = is_optional_impl<std::remove_cvref_t<T>>;

using KeyedArgs = std::unordered_map<std::string, std::vector<std::string>>;

template<typename T>
T parse_scalar(const std::string& s)
{
    if constexpr (std::same_as<T, std::string>) {
        return s;
    } else {
        T v{};
        auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), v);
        if (err != std::errc() || ptr != s.data() + s.size())
            throw std::invalid_argument(std::format("Failed to parse: {}", s));
        return v;
    }
}


template<typename... Ts>
auto parse_args(const std::vector<std::string>& args) {
    std::size_t idx = 0;

    auto parse_one = [&](auto tag) {
        using U = typename decltype(tag)::type;

        if constexpr (is_optional<U>) {
            if (idx < args.size())
                return U{ parse_scalar<typename U::value_type>(args[idx++]) };
            else
                return U{};
        } else {
            // mandatory
            if (idx >= args.size())
                throw std::invalid_argument(std::format("Missing argument at position {}", idx));
            return parse_scalar<U>(args[idx++]);
        }
    };

    auto parsed_args = std::make_tuple(parse_one(std::type_identity<Ts>{})...);

    std::vector<std::string> extra_args;
    if (idx < args.size()) {
        extra_args.assign(args.begin() + idx, args.end());
    }

    return std::tuple_cat(parsed_args, std::tuple{ std::move(extra_args) });
}

template<typename K, typename T>
T get_or_default(const std::unordered_map<K, T>& map, const K& key, const T& default_) {
    auto it = map.find(key);
    if (it == map.end()) {
        return default_;
    }
    return it->second;
}

KeyedArgs group_by_keywords(const std::vector<std::string>& args, const std::vector<std::string>& keywords);
std::vector<std::string> split(const std::string& line);
std::string join(const std::vector<std::string>& words, const std::string& delimiter=" ");
bool contains(const std::vector<std::string>& words, const std::string& value);

} // bears_chess
