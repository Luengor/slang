cmake_minimum_required(VERSION 3.28)
project(ci-again)

# things
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(DEBUG "Don't define NDEBUG and disable optimizations" ON)

set(TARGET_NAME cia)

set(SOURCES
    src/main.cpp
    src/chunk.cpp
    src/value.cpp
    src/vm.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

# Global stuff
if (${DEBUG})
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -O0 -g)
    target_link_options(${TARGET_NAME} PRIVATE -Wall -O0 -g)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -O3)
    target_link_options(${TARGET_NAME} PRIVATE -O3)
    target_compile_definitions(${TARGET_NAME} PRIVATE NDEBUG)
endif()


