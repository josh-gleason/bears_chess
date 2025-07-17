cmake_minimum_required(VERSION 3.15)
project(bears_chess
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Add optimization flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mtune=native")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mpopcnt -mavx2 -msse4.1 -mssse3 -msse3 -msse2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native -msse2 -msse3 -mssse3 -msse4.1 -mpopcnt -mavx2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mtune=native -msse2 -msse3 -mssse3 -msse4.1 -mpopcnt -mavx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native -msse2 -msse3 -mssse3 -msse4.1 -mpopcnt -mavx")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fno-inline")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fconstexpr-ops-limit=1000000000 -fconstexpr-loop-limit=10000000 -ffunction-sections -fdata-sections -flto)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fconstexpr-depth=1000000000 -fconstexpr-steps=1000000000 -ffunction-sections -fdata-sections)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)

add_subdirectory(src)
add_subdirectory(tools)

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
