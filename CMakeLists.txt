cmake_minimum_required(VERSION 3.19)
project(bears_chess
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
        add_compile_options(
            -O3
            -DNDEBUG
            -flto=thin
        )
        add_link_options(
            -fuse-ld=lld
            -Wl,--gc-sections
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
    SYSTEM
)

FetchContent_MakeAvailable(spdlog)

# torch

option(BUILD_TORCH_EVALUATOR "Build TorchScript evaluator (Requires Python torch package)" OFF)

if(BUILD_TORCH_EVALUATOR)
    find_package(Python 3.10 COMPONENTS Interpreter REQUIRED)
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import torch, os; print(os.path.dirname(torch.__file__), end='')"
        OUTPUT_VARIABLE TORCH_DIR
        RESULT_VARIABLE TORCH_QUERY_FAILED
    )
    if(TORCH_QUERY_FAILED)
        message(FATAL_ERROR "BUILD_TORCH_EVALUATOR is ON but 'import torch' failed for ${Python_EXECUTABLE}")
    endif()

    add_library(torch_runtime INTERFACE)
    target_include_directories(torch_runtime SYSTEM INTERFACE
        ${TORCH_DIR}/include
        ${TORCH_DIR}/include/torch/csrc/api/include
    )
    target_compile_definitions(torch_runtime INTERFACE _GLIBCXX_USE_CXX11_ABI=1)
    target_link_directories(torch_runtime INTERFACE ${TORCH_DIR}/lib)
    target_link_options(torch_runtime INTERFACE -Wl,--no-as-needed -Wl,-rpath,${TORCH_DIR}/lib)
    target_link_libraries(torch_runtime INTERFACE torch torch_cpu c10)
    if(EXISTS ${TORCH_DIR}/lib/libtorch_cuda.so)
        target_link_libraries(torch_runtime INTERFACE torch_cuda c10_cuda)
    endif()
endif()

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
        SYSTEM
    )

    # prevent googletest from adding its own tests
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_PYTHON_BINDINGS "Build the bears_chess python module" OFF)
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings)
endif()