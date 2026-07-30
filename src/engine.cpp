#include <stdexcept>
#include <format>
#include <string>
#include <algorithm>
#include "engine.hpp"
#include "log.hpp"
#include "movegen.hpp"
#include "board_utils.hpp"

namespace bears_chess {

static const int DEFAULT_DEPTH = 245;
static const int INFINITE_DEPTH = INT32_MAX;

using log::uci_print, log::uci_println, log::uci_info;

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
    log::set_uci_mode(true);

    uci_println("id name Bear's Chess Engine");
    uci_println("id author Josh Gleason");

    for (const auto& [name, opt] : options) {
        uci_print("option name {} type ", name);

        switch (opt.type) {
            case uci::OptionType::Check: {
                uci_println("check default {}", (std::get<bool>(opt.value) ? "true" : "false"));
                break;
            }
            case uci::OptionType::Spin: {
                int def = std::get<int>(opt.value);
                const auto& meta = std::get<uci::SpinBounds>(opt.meta);
                uci_println("spin default {} min {} max {}", def, meta.min, meta.max);
                break;
            }
            case uci::OptionType::Combo: {
                std::string def = std::get<std::string>(opt.value);
                uci_print("combo default {}", def);
                const auto& meta = std::get<uci::ComboOptions>(opt.meta);
                for (auto &choice : meta.allowed) {
                    uci_print(" var {}", choice);
                }
                uci_println("");
                break;
            }
            case uci::OptionType::Button: {
                uci_println("button");
                break;
            }
            case uci::OptionType::String: {
                std::string def = std::get<std::string>(opt.value);
                if (def.empty())
                    uci_println("string default <empty>");
                else
                    uci_println("string default {}", def);
                break;
            }
        }
    }

    uci_println("uciok");
}

void Engine::ucinewgame() {
    board = Board();
    log::debug("Resetting engine state for a new game");
}

static void log_go_options(const Engine::GoOptions& opts) {
    log::debug("Starting search with options:");
    if (opts.searchmoves.has_value()) {
        log::debug("  searchmoves: {} moves specified", opts.searchmoves->size());
    }
    if (opts.ponder) {
        log::debug("  ponder mode enabled");
    }
    if (opts.wtime.has_value()) {
        log::debug("  wtime: {} ms", *opts.wtime);
    }
    if (opts.btime.has_value()) {
        log::debug("  btime: {} ms", *opts.btime);
    }
    if (opts.winc.has_value()) {
        log::debug("  winc: {} ms", *opts.winc);
    }
    if (opts.binc.has_value()) {
        log::debug("  binc: {} ms", *opts.binc);
    }
    if (opts.movestogo.has_value()) {
        log::debug("  movestogo: {}", *opts.movestogo);
    }
    if (opts.depth.has_value()) {
        log::debug("  depth: {}", *opts.depth);
    }
    if (opts.nodes.has_value()) {
        log::debug("  nodes: {}", *opts.nodes);
    }
    if (opts.movetime.has_value()) {
        log::debug("  movetime: {} ms", *opts.movetime);
    }
    if (opts.infinite) {
        log::debug("  infinite search enabled");
    }
}

MoveList filter_moves(const MoveList& original_moves, const std::vector<Move>& allowed_moves) {
    MoveList filtered_moves{};
    for (const Move& move : original_moves) {
        if (std::find(allowed_moves.cbegin(), allowed_moves.cend(), move) != allowed_moves.cend()) {
            filtered_moves.emplace_back(move);
        }
    }
    return filtered_moves;
}

void Engine::go(const GoOptions& opts) {
    log_go_options(opts);

    MoveList moves = generate_moves<bears_chess::LegalPolicy>(board);

    if (opts.searchmoves.has_value()) {
        moves = filter_moves(moves, *opts.searchmoves);
    }

    if (moves.empty()) {
        log::uci_println("bestmove 0000");
        return;
    }

    Move best_move{ Square::NONE, Square::NONE, MoveType::NONE };

    int max_depth = DEFAULT_DEPTH;
    if (opts.depth.has_value()) {
        max_depth = *opts.depth;
    } else if (opts.infinite) {
        max_depth = INFINITE_DEPTH;
    }

    uci_info("Starting iterative deepening search with max depth {}", max_depth);
   
    const MoveList principal_variation{};

    for (int depth = 1; depth <= max_depth; ++depth) {
        // TODO iterative deepning
    }

    if (best_move.move_type == MoveType::NONE) {
        log::debug("Search failed to find a best move, using first legal move");
        best_move = *moves.begin();
    }

    Move ponder_response{ Square::NONE, Square::NONE, MoveType::NONE };
    if (opts.ponder) {
        // TODO: temporarily just pick the first move after best_move for ponder, actually use second move of PV
        UndoInfo undo_info = board.do_move(best_move);
        MoveList response_moves = generate_moves<bears_chess::LegalPolicy>(board);
        if (response_moves.size() > 0) {
            ponder_response = *response_moves.begin();
        }
        board.undo_move(undo_info);
    }

    if (ponder_response.move_type == MoveType::NONE) {
        log::uci_print("bestmove {}\n", convert_move_to_uci(best_move));
    } else {
        log::uci_print(
            "bestmove {} ponder {}\n",
            convert_move_to_uci(best_move),
            convert_move_to_uci(ponder_response)
        );
    }
}

void Engine::stop() {
    // signal the search to stop as soon as possible
    log::debug("Stop command received - stopping search");

    // TODO
    // 1. Set a stop flag that is checked by the search algorithm
    // 2. Wait for the search to finish (or force stop after timeout)
    // 3. Ensure that bestmove is sent after stopping
}

void Engine::ponderhit() {
    // this is called when the opponent has made the move we were pondering on
    log::debug("Ponderhit command received - opponent played expected move");

    // TODO
    // 1. Transition from pondering mode to normal search mode
    // 2. Keep the current search results instead of starting from scratch
    // 3. Start counting down our own time instead of using ponder time
    // 4. Ensure the search will produce a bestmove when finished
}

void Engine::debug(bool on) {
    debug_on = on;
    log::set_uci_debug(on);
}

bool Engine::debug() const {
    return debug_on;
}

static bool is_uci_prefixed(std::string_view name) {
    return name.size() >= 4 && std::tolower(name[0]) == 'u' && std::tolower(name[1]) == 'c' && std::tolower(name[2]) == 'i' && name[3] == '_';
}

void Engine::set_option(const std::string& name, const uci::RawOptionValue& value) {
    if (!options.contains(name)) {
        // ignore UCI_* commands that are not registered
        if (!is_uci_prefixed(name)) {
            throw std::invalid_argument(std::format("Unknown option {}", name));
        }
        log::debug("Unknown UCI option provided: {}", name);        
        return;
    }
    options[name].value = uci::convert_raw(value, options[name].type);
    options[name].on_change();
}

void Engine::isready() const
{
    uci_println("readyok");
}

void Engine::handle_hash_opt() {
    int hash_size_mb = std::get<int>(options["Hash"].value);
    log::debug("Setting hash size to {} MB", hash_size_mb);

    // TODO
    // 1. Allocate/resize the transposition table based on the new size
    // 2. Clear the transposition table
    // 3. Update any related data structures that depend on the hash size
}

void Engine::handle_ponder_opt() {
    bool ponder_enabled = std::get<bool>(options["Ponder"].value);
    log::debug("Ponder option set to {}", ponder_enabled ? "true" : "false");

    // TODO
    // 1. Update the search behavior to ponder (think on opponent's time) or not
    // 2. The engine should NOT automatically start pondering when this is enabled
    //    It should wait for the "go ponder" command
}

void Engine::handle_multipv_opt() {
    int multipv = std::get<int>(options["MultiPV"].value);
    log::debug("MultiPV option set to {}", multipv);

    // TODO
    // 1. Configure the search to find the top N lines instead of just the best one
    // 2. Update any data structures that track multiple principal variations
    // 3. Ensure the search reports all N principal variations in info output
}

} // bears_chess
