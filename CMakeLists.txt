cmake_minimum_required(VERSION 3.19)
project(bears_chess
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting default build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fconstexpr-ops-limit=1000000000 -fconstexpr-loop-limit=10000000)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -fconstexpr-depth=1000000000
        -fconstexpr-steps=1000000000
        -march=native
        -mtune=native
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_POSITION_INDEPENDENT_CODE OFF CACHE BOOL "" FORCE)

        add_compile_options(
            -O3
            -DNDEBUG
            -flto=thin
            -fno-pie
        )
        add_link_options(
            -fuse-ld=lld
            -no-pie
            -Wl,--gc-sections
            -Wl,--emit-relocs
            -Wl,--thinlto-jobs=${CMAKE_JOB_POOL_COMPILE}
        )
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)

# spdlog

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
)

FetchContent_MakeAvailable(spdlog)

# add subdirs

add_subdirectory(src)
add_subdirectory(tools)

# add tests

option(BUILD_TESTING "Build unit tests with GoogleTest" OFF)
if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
    )

    # prevent googletest from adding its own tests
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
