add_executable(perft
    perft.cpp
)
target_link_libraries(perft PRIVATE bears_chess)

add_executable(make_magic
    make_magic.cpp
)
target_link_libraries(make_magic PRIVATE bears_chess)

add_executable(cli
    cli.cpp
)
target_link_libraries(cli PRIVATE bears_chess)

# add_subdirectory(benchmarks)
