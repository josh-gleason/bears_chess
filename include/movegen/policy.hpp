#pragma once
#include "bitboard.hpp"

namespace bears_chess {

template<typename T>
concept MoveGenPolicy = requires(const T& policy) {
    { T::enforce_king_safety } -> std::convertible_to<bool>;
    { T::enforce_evasions } -> std::convertible_to<bool>;
    { T::enforce_pins } -> std::convertible_to<bool>;
};

} // namespace bears_chess
