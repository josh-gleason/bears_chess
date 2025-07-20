add_executable(perft
    perft.cpp
)
target_link_libraries(perft PRIVATE bears_chess)

# overwrite the perft executable with the BOLT optimized version
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PERF_DATA "${CMAKE_BINARY_DIR}/perft.perf.data")
    set(PERF_FDATA "${CMAKE_BINARY_DIR}/perft.perf.fdata")

    set(BOLT_ARGS
        -data=${PERF_FDATA}
        -reorder-blocks=ext-tsp
        -split-functions
        -split-all-cold
        -dyno-stats
    )

    add_custom_target(perft-bolt ALL
        COMMAND perf record -o ${PERF_DATA} -e cycles:u -c 1000000 -- $<TARGET_FILE:perft>
        COMMAND perf2bolt -nl -p ${PERF_DATA} -o ${PERF_FDATA} $<TARGET_FILE:perft>
        COMMAND llvm-bolt $<TARGET_FILE:perft> -o $<TARGET_FILE:perft>.bolt ${BOLT_ARGS}
        COMMENT "optimizing perft with BOLT"
        DEPENDS perft
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CALLGRIND_ARGS
        --callgrind-out-file=${CMAKE_BINARY_DIR}/perft.callgrind.out
    )

    add_custom_target(perft-callgrind ALL
        COMMAND valgrind --tool=callgrind ${CALLGRIND_ARGS} $<TARGET_FILE:perft>
        COMMENT "running perft with callgrind, call `kcachegrind ${CMAKE_BINARY_DIR}/perft.callgrind.out` to view results"
        DEPENDS perft
    )
endif()

add_executable(make_magic
    make_magic.cpp
)
target_link_libraries(make_magic PRIVATE bears_chess)

# add_subdirectory(benchmarks)
