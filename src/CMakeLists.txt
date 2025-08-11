set(SOURCES
    board.cpp
    board_utils.cpp
    bitboard.cpp
    evaluation_utils.cpp
    cli.cpp
    engine.cpp
    parse_utils.cpp
    uci_options.cpp
    log.cpp
)

add_library(bears_chess ${SOURCES})

target_include_directories(bears_chess
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${ASIO_INCLUDE_DIR}
)

target_link_libraries(bears_chess PRIVATE spdlog::spdlog)
