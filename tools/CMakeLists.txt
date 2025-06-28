add_executable(perft
    perft.cpp
)
target_link_libraries(perft PRIVATE bears_chess)

add_executable(make_magic
    make_magic.cpp
)
target_link_libraries(make_magic PRIVATE bears_chess)

target_compile_options(make_magic PRIVATE -fconstexpr-ops-limit=1000000000 -fconstexpr-loop-limit=10000000)

# add_subdirectory(benchmarks)
