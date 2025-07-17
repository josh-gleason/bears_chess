set(SOURCES
    board.cpp
    board_utils.cpp
    checkmate.cpp
    magics.cpp
    legal_movegen.cpp
    pseudolegal_movegen.cpp
)

add_library(bears_chess ${SOURCES})

target_include_directories(bears_chess
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# target_compile_options(bears_chess PRIVATE -fconstexpr-ops-limit=1000000000 -fconstexpr-loop-limit=10000000)
