
cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/xtensa.cmake")

project(module_example)

set(MODULES_LIST module_example)

include(../../cmake/build_library.cmake)
