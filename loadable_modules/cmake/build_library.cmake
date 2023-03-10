
#set(ARCH xtensa)
#set(TOOLCHAIN xt)
#include(${CMAKE_CURRENT_LIST_DIR}/../../scripts/cmake/${ARCH}-toolchain.cmake)

set(XTENSA_CORE_OPT "--xtensa-core=$ENV{XTENSA_CORE}")
set(RIMAGE_INCLUDE_DIR "../../../rimage/src/include")
cmake_path(ABSOLUTE_PATH RIMAGE_INCLUDE_DIR NORMALIZE)
message(STATUS ${RIMAGE_INCLUDE_DIR})

add_custom_target(ld_script ALL
	COMMAND ${CMAKE_COMMAND} -DLIBNAME=${LIBRARY} -P ${CMAKE_CURRENT_LIST_DIR}/ldscripts.cmake
	COMMENT "Make beautiful linker script"
)
#execute_process(COMMAND ${CMAKE_COMMAND} -DLIBNAME=${LIBRARY} -P ${CMAKE_CURRENT_LIST_DIR}/ldscripts.cmake)

foreach(MODULE ${MODULES_LIST})
	add_executable(${MODULE})
	set_target_properties(${MODULE} PROPERTIES OUTPUT_NAME "${MODULE}.mod")

	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../modules/${MODULE} ${MODULE}_module)
	target_include_directories(${MODULE} PRIVATE "../../src/include/sof/audio/module_adapter/iadk"
		${RIMAGE_INCLUDE_DIR}
	)
	add_dependencies(${MODULE} ld_script)
	target_link_options(${MODULE} PRIVATE
		${XTENSA_CORE_OPT}
		### Maybe this is not necessary if proper linker script is provided???!!!
		"-nostdlib" "-nodefaultlibs" "--verbose"
		"-Wl,--no-undefined" "-Wl,--unresolved-symbols=report-all" "-Wl,--error-unresolved-symbols"
		## "-Wl,--gc-sections"
		"-Wl,-Map,$<TARGET_FILE:${MODULE}>.map"
		## "-mlsp=."
		"-T" "elf32xtensa.x"
		#"-T" "${CMAKE_CURRENT_LIST_DIR}/elf32xtensa.x"
	)
endforeach()
