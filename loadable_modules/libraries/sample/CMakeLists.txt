
cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/xtensa.cmake")

project(sample)
set(MODULES_LIST sample sample2)

include(../../cmake/build_library.cmake)
