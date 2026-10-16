cmake_minimum_required(VERSION 3.16)

project(cpp_lox_interpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lox src/main.cpp)
target_include_directories(lox PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/lox")

add_executable(ast_printer_test src/test.cpp)
target_include_directories(ast_printer_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/lox")
