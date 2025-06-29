set(SOURCES
    board.cpp
    board_utils.cpp
    log.cpp
    movegen.cpp
)

add_library(bears_chess ${SOURCES})

target_include_directories(bears_chess
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# spdlog
if (NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

target_link_libraries(bears_chess
    PUBLIC spdlog::spdlog
)

target_compile_definitions(bears_chess PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
    $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=6>
)

target_compile_options(bears_chess PRIVATE -fconstexpr-ops-limit=1000000000 -fconstexpr-loop-limit=10000000)
