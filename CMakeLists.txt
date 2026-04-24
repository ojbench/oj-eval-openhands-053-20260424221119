cmake_minimum_required(VERSION 3.10)
project(problem_053)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(code main.cpp)
# Optimize a bit
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(code PRIVATE -O2 -Wall -Wextra -Wno-unused-parameter)
endif()
