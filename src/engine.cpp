#include "engine.hpp"

#include "log.hpp"
#include "movegen.hpp"
#include "board_utils.hpp"
#include "search.hpp"

#include <stdexcept>
#include <format>
#include <string>
#include <thread>

namespace bears_chess {

using log::uci_print, log::uci_println, log::uci_info;

constexpr int64_t SAFETY_MARGIN_MS = 50;
constexpr int64_t NO_CLOCK_FALLBACK_MS = 2000;

Engine::Engine() :
    options{
        { "Hash", { std::bind_front(&Engine::handle_hash_opt, this), uci::OptionType::Spin, 16, uci::SpinBounds{1, 1048576} } },
        { "Ponder", { std::bind_front(&Engine::handle_ponder_opt, this), uci::OptionType::Check, false } },
        { "MultiPV", { std::bind_front(&Engine::handle_multipv_opt, this), uci::OptionType::Spin, 1, uci::SpinBounds{1, 256} } },
    },
    search(16, std::bind_front(&Engine::search_report, this))
{
    for (auto& [name, opt] : options) {
        if (!std::holds_alternative<std::monostate>(opt.value)) {
            opt.on_change();
        }
    }
}

Engine::~Engine() {
    wait_for_search();
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
    wait_for_search();
    set_position(Board());
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


std::optional<std::chrono::steady_clock::time_point> Engine::deadline_from_go_opts(const GoOptions& opts) const {
    if (opts.movetime) {
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(*opts.movetime);
    } else if (opts.infinite) {
        return std::nullopt;
    }

    const bool white = (board.side_to_move == Color::WHITE);
    const std::optional<int64_t> time_remaining = white ? opts.wtime : opts.btime;
    const int64_t increment = (white ? opts.winc : opts.binc).value_or(0);

    if (!time_remaining) {
        if (opts.depth || opts.nodes) {
            return std::nullopt;
        }
        uci_info("WARNING: go with no lcok or other limit; using {} ms fallback", NO_CLOCK_FALLBACK_MS);
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(NO_CLOCK_FALLBACK_MS);
    }

    // TODO: better default later
    int64_t ub = std::max<int64_t>(1, *time_remaining - SAFETY_MARGIN_MS);
    int64_t ms_remaining = std::clamp<int64_t>(*time_remaining / 30 + increment, 1, ub);

    return std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_remaining);
}

void Engine::go(const GoOptions& opts) {
    // TODO: handle ponder properly

    wait_for_search();

    log_go_options(opts);
    SearchOptions search_opts{};

    search_opts.max_depth = opts.depth;
    search_opts.max_node_count = opts.nodes;
    search_opts.searchmoves = opts.searchmoves;
    search_opts.num_pvs = num_pvs;

    if (search_opts.max_depth.has_value()) {
        uci_info("search max depth set to {}", *search_opts.max_depth);
    }

    if (search_opts.max_node_count.has_value()) {
        uci_info("search max node count set to {}", *search_opts.max_node_count);
    }

    if (!search_opts.max_depth.has_value() && !search_opts.max_node_count.has_value()) {
        uci_info("running infinite search");
    }

    search_opts.deadline = deadline_from_go_opts(opts);

    auto search_fun = ([](
        std::stop_token stop_token,
        Search& search,
        Board board,
        std::vector<ZobristHash> hash_history,
        SearchOptions search_opts
    ) {
        MoveList moves = generate_moves<bears_chess::LegalPolicy>(board);
        Move best_move = MOVE_NONE;
        Move ponder_response = MOVE_NONE;

        if (moves.empty()) {
            log::uci_println("bestmove 0000");
            return;
        }

        auto search_results = search.go(
            stop_token,
            board,
            hash_history,
            search_opts
        );

        if (!search_results.principal_variations.empty()) {
            const auto& pv = search_results.principal_variations.front();
            if (!pv.moves.empty()) {
                best_move = pv.moves.front();
            }
            if (pv.moves.size() > 1) {
                ponder_response = pv.moves[1];
            }
        }

        if (best_move.is_none() && !moves.empty()) {
            log::debug("Search failed to find a best move, using first legal move");
            best_move = *moves.begin();
        }

        if (ponder_response.is_none()) {
            log::uci_println("bestmove {}", convert_move_to_uci(best_move));
        } else if (!best_move.is_none()) {
            log::uci_println(
                "bestmove {} ponder {}",
                convert_move_to_uci(best_move),
                convert_move_to_uci(ponder_response)
            );
        } else {
            uci_println("bestmove 0000");
        }
    });

    search_thread = std::jthread(
        search_fun,
        std::ref(search),
        board,
        hash_history,
        search_opts
    );
}

void Engine::stop() {
    uci_info("Stop command received - stopping search");
    wait_for_search();
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

void Engine::isready() const {
    uci_println("readyok");
}

const Board& Engine::position() const {
    return board;
}

void Engine::set_position(Board new_board) {
    wait_for_search();
    hash_history.clear();
    board = new_board;
}

void Engine::play_move(Move move) {
    wait_for_search();
    hash_history.push_back(board.hash);
    board.do_move(move);
}

void Engine::wait_for_search() {
    if (search_thread.joinable()) {
        search_thread.request_stop();
        search_thread.join();
    }
}

void Engine::search_report(const Search::SearchResult& result, std::chrono::milliseconds elapsed, int hashfull) const {
    uint64_t ms = elapsed.count();
    uint64_t nps = (ms > 0) ? (result.nodes * 1000 / ms) : 0;

    const auto& principal_variations = result.principal_variations;
    for (size_t pv_idx = 0; pv_idx < principal_variations.size(); ++pv_idx) {
        const auto& pv = principal_variations[pv_idx];

        std::string score;
        if (pv.score >= MATE_SCORE_BOUND) {
            int plies = SCORE_INF - 1 - pv.score;
            score = std::format("mate {}", (plies + 1) / 2);
        } else if (pv.score <= -MATE_SCORE_BOUND) {
            int plies = pv.score + SCORE_INF - 1;
            score = std::format("mate -{}", (plies + 1) / 2);
        } else {
            score = std::format("cp {}", pv.score);
        }

        std::string pv_msg;
        for  (const Move& move : pv.moves) {
            if (!pv_msg.empty()) {
                pv_msg += " ";
            }
            pv_msg += convert_move_to_uci(move);
        }

        size_t multipv = pv_idx + 1;
        
        uci_println(
            "info depth {} multipv {} score {} nodes {} nps {} time {} hashfull {} pv {}",
            result.depth, multipv, score, result.nodes, nps, ms, hashfull, pv_msg
        );
    }
}

void Engine::handle_hash_opt() {
    wait_for_search();

    int hash_size_mb = std::get<int>(options["Hash"].value);
    uci_info("Setting hash size to {} MB", hash_size_mb);
    search.resize_tt(hash_size_mb);
}

void Engine::handle_ponder_opt() {
    bool ponder_enabled = std::get<bool>(options["Ponder"].value);
    uci_info("Ponder option set to {}", ponder_enabled ? "true" : "false");
}

void Engine::handle_multipv_opt() {
    int multipv = std::get<int>(options["MultiPV"].value);
    num_pvs = std::clamp<int>(multipv, 1, MAX_PLY);
    uci_info("MultiPV option set to {}", num_pvs);
}

} // bears_chess
