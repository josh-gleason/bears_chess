#pragma once
#include "bitboard.hpp"

namespace bears_chess {

template<typename T>
concept MoveGenPolicy = requires(const T& policy) {
    { T::enforce_king_safety } -> std::convertible_to<bool>;
    { T::enforce_evasions } -> std::convertible_to<bool>;
    { T::enforce_pins } -> std::convertible_to<bool>;

    requires (!T::enforce_king_safety) || requires {
        { T::king_unallowed } -> std::same_as<Bitboard&>;
    };
    requires (!T::enforce_evasions) || requires {
        { T::evasion_mask } -> std::same_as<Bitboard&>;
        { T::checkers } -> std::same_as<Bitboard&>;
    };
    requires (!T::enforce_pins) || requires {
        { T::pinned } -> std::same_as<Bitboard&>;
        { T::pin_rays } -> std::same_as<Bitboard(&)[num_of<IndexDirection>]>;
    };
};

} // namespace bears_chess
