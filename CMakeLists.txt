cmake_minimum_required(VERSION 3.28)
project(ci-again)

# things
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(DEBUG_PRINT "Enable debug printing" OFF)
option(RELEASE "Define NDEBUG and enable optimizations" OFF)

set(TARGET_NAME cia)

set(SOURCES
    src/ast_core.cpp
    src/ast_expr.cpp
    src/ast_stmt.cpp
    src/ast_type.cpp
    src/chunk.cpp
    src/compiler.cpp
    src/compile_context.cpp
    src/error.cpp
    src/native.cpp
    src/object.cpp
    src/parser.cpp
    src/scanner.cpp
    src/types.cpp
    src/value.cpp
    src/vm.cpp
)

add_executable(${TARGET_NAME} ${SOURCES} src/main.cpp)

# Global stuff
if (${DEBUG_PRINT})
    target_compile_definitions(${TARGET_NAME} PRIVATE DEBUG_PRINT)
endif()

if (${RELEASE})
    target_compile_options(${TARGET_NAME} PRIVATE -O3)
    target_link_options(${TARGET_NAME} PRIVATE -O3)
    target_compile_definitions(${TARGET_NAME} PRIVATE NDEBUG)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -O0 -g)
    target_link_options(${TARGET_NAME} PRIVATE -Wall -O0 -g)
endif()


