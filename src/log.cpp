#include "log.hpp"

void init_log()
{
    #if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
        spdlog::set_level(spdlog::level::debug);
    #endif
}
