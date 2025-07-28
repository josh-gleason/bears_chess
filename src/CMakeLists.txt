set(SOURCES
    board.cpp
    board_utils.cpp
    bitboard.cpp
    evaluation_utils.cpp
    cli.cpp
    engine.cpp
    parse_utils.cpp
    uci_options.cpp
)

add_library(bears_chess ${SOURCES})

target_include_directories(bears_chess
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${ASIO_INCLUDE_DIR}
)
