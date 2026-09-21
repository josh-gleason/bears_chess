find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)

include(FetchContent)
FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind
    GIT_TAG        v3.0.1
    SYSTEM
)
FetchContent_MakeAvailable(nanobind)

nanobind_add_module(bears_chess_py
    module.cpp
    attributes.cpp
    move.cpp
    board.cpp
    search.cpp
    mcts.cpp
    encoding.cpp
)
target_link_libraries(bears_chess_py PRIVATE bears_chess)
set_target_properties(bears_chess_py PROPERTIES
    OUTPUT_NAME bears_chess
)

nanobind_add_stub(
    bears_chess_stub
    MODULE bears_chess
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bears_chess.pyi
    PYTHON_PATH ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS bears_chess_py
)

file(GENERATE
    OUTPUT ${PROJECT_SOURCE_DIR}/python/tests/_module_location.py
    CONTENT "MODULE_DIR = r\"$<TARGET_FILE_DIR:bears_chess_py>\"\n"
)

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import pytest"
    RESULT_VARIABLE _pytest_missing
    OUTPUT_QUIET ERROR_QUIET
)
if(_pytest_missing EQUAL 0)
    add_test(NAME python_bindings
        COMMAND Python::Interpreter -m pytest ${PROJECT_SOURCE_DIR}/python/tests -q
    )
    set_tests_properties(python_bindings PROPERTIES
        ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:bears_chess_py>"
    )
endif()
