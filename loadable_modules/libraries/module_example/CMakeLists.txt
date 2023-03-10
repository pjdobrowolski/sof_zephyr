
cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/xtensa.cmake")

set(ZEPHYR_BASE "../../../../zephyr")
add_definitions( -DMAJOR_IADSP_API_VERSION=5 )
add_definitions( -DMIDDLE_IADSP_API_VERSION=0 )
add_definitions( -DMINOR_IADSP_API_VERSION=0 )
add_definitions( -imacros${ZEPHYR_BASE}/include/zephyr/toolchain/xcc_missing_defs.h )

project(module_example)

list(APPEND MODULES_LIST module_example)
list(APPEND MODULES_LIST module_example2)

include(../../cmake/build_library.cmake)

foreach(MODULE ${MODULES_LIST})
	target_include_directories(${MODULE} PUBLIC "../../../../zephyr/lib/libc/minimal/include"
	"../../../../zephyr/include/zephyr/toolchain")
endforeach()
