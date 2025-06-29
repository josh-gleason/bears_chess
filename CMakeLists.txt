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

# # Define optimization macros for the benchmark
# add_definitions(-DUSE_POPCNT -DUSE_AVX2 -DUSE_SSE41 -DUSE_SSSE3 -DUSE_SSE2)

# Optimization configuration
option(ENABLE_POPCNT "Enable POPCNT instruction" ON)
option(ENABLE_AVX2 "Enable AVX2 instructions" ON)
option(ENABLE_SSE41 "Enable SSE4.1 instructions" ON)
option(ENABLE_SSSE3 "Enable SSSE3 instructions" ON)
option(ENABLE_SSE3 "Enable SSE3 instructions" ON)
option(ENABLE_SSE2 "Enable SSE2 instructions" ON)

# Set optimization flags based on options
set(OPTIMIZATION_FLAGS "-O4 -mtune=native")

if(ENABLE_SSE2)
    list(APPEND OPTIMIZATION_FLAGS "-msse2")
    add_definitions(-DUSE_SSE2)
endif()

if(ENABLE_SSE3)
    list(APPEND OPTIMIZATION_FLAGS "-msse3")
    add_definitions(-DUSE_SSE3)
endif()

if(ENABLE_SSSE3)
    list(APPEND OPTIMIZATION_FLAGS "-mssse3")
    add_definitions(-DUSE_SSSE3)
endif()

if(ENABLE_SSE41)
    list(APPEND OPTIMIZATION_FLAGS "-msse4.1")
    add_definitions(-DUSE_SSE41)
endif()

if(ENABLE_POPCNT)
    list(APPEND OPTIMIZATION_FLAGS "-mpopcnt")
    add_definitions(-DUSE_POPCNT)
endif()

if(ENABLE_AVX2)
    list(APPEND OPTIMIZATION_FLAGS "-mavx2")
    add_definitions(-DUSE_AVX2)
endif()

# Apply optimization flags
string(REPLACE ";" " " OPTIMIZATION_FLAGS_STRING "${OPTIMIZATION_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OPTIMIZATION_FLAGS_STRING}")

# Print configuration
message(STATUS "Optimization flags: ${OPTIMIZATION_FLAGS_STRING}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.15.3
)
FetchContent_MakeAvailable(spdlog)

add_subdirectory(src)
add_subdirectory(tools)

option(BUILD_TESTING "Build unit tests with GoogleTest" ON)

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
