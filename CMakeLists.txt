cmake_minimum_required(VERSION 3.16)

project(raven VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT UNIX)
    message(FATAL_ERROR "raven requires a POSIX system")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(raven_flags INTERFACE)
target_compile_options(raven_flags INTERFACE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address,undefined>
)
target_link_options(raven_flags INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_library(raven_core STATIC
    src/instruction.c
    src/format_instruction.c
    src/cpu.c
    src/bus.c
    src/loader.c
    src/execute.c
)
target_include_directories(raven_core PUBLIC src src/common)
target_link_libraries(raven_core PUBLIC raven_flags)

include(FetchContent)
FetchContent_Declare(bitcast
    GIT_REPOSITORY https://github.com/vincent-lafouasse/libbitcast.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(bitcast)
target_link_libraries(raven_core PRIVATE bitcast::bitcast)

add_executable(raven     src/main.c)
add_executable(raven-dis tools/disassemble.c)

target_link_libraries(raven     PRIVATE raven_core)
target_link_libraries(raven-dis PRIVATE raven_core bitcast::bitcast)

SET(TESTING OFF CACHE BOOL "compile tests")
if (TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
