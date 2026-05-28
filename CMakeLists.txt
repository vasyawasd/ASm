cmake_minimum_required(VERSION 3.10)
project(vmovdqa64_encoder C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(vmov_parser main.c)
target_compile_options(vmov_parser PRIVATE -Wall -Wextra -O3)

# ASAN build
add_executable(vmov_parser_asan main.c)
target_compile_options(vmov_parser_asan PRIVATE -Wall -Wextra -g -fsanitize=address,undefined)
target_link_options(vmov_parser_asan PRIVATE -fsanitize=address,undefined)

# Tests run
enable_testing()
add_test(NAME built_in_tests COMMAND vmov_parser --test)
