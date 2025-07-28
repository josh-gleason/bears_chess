#include <vector>
#include <variant>
#include <string>
#include <functional>

namespace bears_chess {

namespace uci {

enum class OptionType {
    Check,
    Spin,
    Combo,
    Button,
    String
};

struct SpinBounds {
    int min;
    int max;
};

struct ComboOptions {
    std::vector<std::string> allowed;
};


using RawOptionValue = std::vector<std::string>;
using OptionValue = std::variant<std::monostate, bool, int, std::string>;
using OptionMetadata = std::variant<std::monostate, SpinBounds, ComboOptions>;

struct Option {
    std::function<void()> on_change;
    OptionType type;
    OptionValue value = std::monostate();
    OptionMetadata meta = std::monostate();
};

OptionValue convert_raw(const RawOptionValue& raw_value, OptionType type);

}   // namespace uci

}   // namespace bears_chess