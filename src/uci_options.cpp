#include "bears_chess/uci_options.hpp"
#include "bears_chess/parse_utils.hpp"

namespace bears_chess {

namespace uci {

OptionValue convert_raw(const RawOptionValue& raw_value, OptionType type)
{
    switch (type) {
        case OptionType::Check:
            return contains(raw_value, "true");
        case OptionType::Button:
            return std::monostate();
        case OptionType::Spin:
            return std::get<0>(parse_args<int>(raw_value));
        case OptionType::String:
        case OptionType::Combo:
            return std::get<0>(parse_args<std::string>(raw_value));
    }

    throw std::logic_error("convert_raw: unhandled OptionType");
}

} // uci

} // bears_chess
