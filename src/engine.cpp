#include <stdexcept>
#include <format>
#include <print>
#include <string>
#include "engine.hpp"

namespace bears_chess {

Engine::Engine() :
    options{
        { "Hash", { [this]() {this->handle_hash_opt();}, uci::OptionType::Spin, 16, uci::SpinBounds{1, 1048576} } },
        { "Ponder", { [this]() {this->handle_ponder_opt();}, uci::OptionType::Check, false } },
        { "MultiPV", { [this]() {this->handle_multipv_opt();}, uci::OptionType::Spin, 1, uci::SpinBounds{1, 256} } },
    }
{
    for (auto& [name, opt] : options) {
        if (!std::holds_alternative<std::monostate>(opt.value)) {
            opt.on_change();
        }
    }
}

void Engine::uci() {
    std::println("id name Bear's Chess Engine");
    std::println("id author Josh Gleason");

    for (const auto& [name, opt] : options) {
        std::print("option name {} type ", name);

        switch (opt.type) {
            case uci::OptionType::Check: {
                std::println("check default {}", (std::get<bool>(opt.value) ? "true" : "false"));
                break;
            }
            case uci::OptionType::Spin: {
                int def = std::get<int>(opt.value);
                const auto& meta = std::get<uci::SpinBounds>(opt.meta);
                std::println("spin default {} min {} max {}", def, meta.min, meta.max);
                break;
            }
            case uci::OptionType::Combo: {
                std::string def = std::get<std::string>(opt.value);
                std::print("combo default {}", def);
                const auto& meta = std::get<uci::ComboOptions>(opt.meta);
                for (auto &choice : meta.allowed) {
                    std::print(" var {}", choice);
                }
                std::println("");
                break;
            }
            case uci::OptionType::Button: {
                std::println("button");
                break;
            }
            case uci::OptionType::String: {
                std::string def = std::get<std::string>(opt.value);
                if (def.empty())
                    std::println("string default <empty>");
                else
                    std::println("string default {}", def);
                break;
            }
        }
    }

    std::println("uciok");
}

void Engine::ucinewgame() {
    // TODO
}

void Engine::go(const GoOptions& opts) {
    // TODO
}

void Engine::stop() {
    // TODO
}

void Engine::ponderhit() {
    // TODO
}

void Engine::debug(bool on) {
    debug_on = true;
}

bool Engine::debug() const {
    return debug_on;
}

void Engine::set_option(const std::string& name, const uci::RawOptionValue& value) {
    if (!options.contains(name)) {
        // ignore UCI_* commands that are not registered
        if (!name.starts_with("UCI_")) {
            throw std::invalid_argument(std::format("Unknown option {}", name));
        }
    }
    options[name].value = uci::convert_raw(value, options[name].type);
    options[name].on_change();
}

void Engine::handle_hash_opt() {
    // TODO: handle hash option change
}

void Engine::handle_ponder_opt() {
    // TODO: handle ponder option change
}

void Engine::handle_multipv_opt() {
    // TODO: handle multipv option change
}

} // bears_chess
