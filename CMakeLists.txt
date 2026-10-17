cmake_minimum_required(VERSION 3.16)

project(raven VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT UNIX)
    message(FATAL_ERROR "raven requires a POSIX system")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(raven_flags INTERFACE)
# Generator expressions holding several flags are written as quoted
# `;`-separated lists.
target_compile_options(raven_flags INTERFACE
    -Wall -Wextra -Wpedantic
    -Wconversion
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
    -Wredundant-decls -Wformat=2 -Wcast-qual -Wpointer-arith -Wwrite-strings
    -Wundef -Wvla -Wnull-dereference -Wdouble-promotion
    # gcc only, clang would warn about unknown options
    "$<$<C_COMPILER_ID:GNU>:-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;-Wjump-misses-init;-Wcast-align=strict>"
    "$<$<C_COMPILER_ID:Clang,AppleClang>:-Wcast-align>"
    # UBSan aborts instead of printing and continuing, so the test runner
    # sees undefined behaviour as a failure
    "$<$<CONFIG:Debug>:-g;-O0;-fno-omit-frame-pointer;-fsanitize=address,undefined;-fno-sanitize-recover=all>"
)
target_link_options(raven_flags INTERFACE
    "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
)

add_library(raven_core STATIC
    src/instruction.c
    src/format_instruction.c
    src/hart.c
    src/bus.c
    src/loader.c
    src/execute.c
    src/env.c
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

# both tests need a RISC-V cross toolchain (riscv64-unknown-elf-*)
enable_testing()
add_test(NAME decoder
    COMMAND sh test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test/decoder-test)
add_test(NAME riscv-tests
    COMMAND ${CMAKE_COMMAND} -E env RAVEN=$<TARGET_FILE:raven> NO_COLOR=1
            ${CMAKE_SOURCE_DIR}/test/run-riscv-tests.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
