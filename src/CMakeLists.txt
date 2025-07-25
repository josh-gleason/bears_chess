set(SOURCES
    board.cpp
    board_utils.cpp
    bitboard.cpp
    evaluation_utils.cpp
    cli.cpp
    async_line_reader.cpp
)

add_library(bears_chess ${SOURCES})

target_include_directories(bears_chess
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${ASIO_INCLUDE_DIR}
)

target_link_libraries(bears_chess PUBLIC asio_standalone)
