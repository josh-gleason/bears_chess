add_executable(bears_chess_tests
    perft_tests.cpp
    make_move_tests.cpp
    zobrist_tests.cpp
    see_tests.cpp
    mcts_tests.cpp
    encoding_tests.cpp
)

target_link_libraries(bears_chess_tests
    PRIVATE
        bears_chess
        gtest_main
)

target_include_directories(bears_chess_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

include(GoogleTest)
gtest_discover_tests(bears_chess_tests)
