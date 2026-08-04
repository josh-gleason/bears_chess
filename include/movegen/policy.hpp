#pragma once
#include "bitboard.hpp"

namespace bears_chess {

template<typename T>
concept LegalityPolicy = requires(const T& policy) {
    { T::enforce_king_safety } -> std::convertible_to<bool>;
    { T::enforce_evasions } -> std::convertible_to<bool>;
    { T::enforce_pins } -> std::convertible_to<bool>;
};

template<typename T>
concept MoveSelection = requires(const T& policy) {
    { T::include_captures } -> std::convertible_to<bool>;
    { T::include_quiets } -> std::convertible_to<bool>;
};

} // namespace bears_chess
