
#set(ARCH xtensa)
#set(TOOLCHAIN xt)
#include(${CMAKE_CURRENT_LIST_DIR}/../../scripts/cmake/${ARCH}-toolchain.cmake)

set(XTENSA_CORE_OPT "--xtensa-core=$ENV{XTENSA_CORE}")

foreach(MODULE ${MODULES_LIST})
    add_library(${MODULE} STATIC)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../modules/${MODULE} ${MODULE}_module)
    target_include_directories(${MODULE} PRIVATE "../../src/include/sof/audio/module_adapter/iadk")

    add_custom_command(TARGET ${MODULE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DFROM=$<TARGET_FILE:${MODULE}> -DTO=$<TARGET_FILE:${MODULE}>.original.a -P ${CMAKE_CURRENT_LIST_DIR}/cp.cmake
        COMMAND ${CROSS_COMPILE}objcopy ${XTENSA_CORE_OPT} --prefix-symbols ${MODULE} --prefix-alloc-sections=.${MODULE} $<TARGET_FILE:${MODULE}>.original.a $<TARGET_FILE:${MODULE}>
        COMMAND ${CROSS_COMPILE}nm ${XTENSA_CORE_OPT} $<TARGET_FILE:${MODULE}> > ${MODULE}.nm
    )

###  add_custom_command(TARGET ${MODULE} POST_BUILD COMMAND ${CROSS_COMPILE}objcopy ${XTENSA_CORE_OPT} --prefix-symbols ${MODULE} --prefix-alloc-sections=.${MODULE} $<TARGET_FILE:${MODULE}> $<TARGET_FILE:${MODULE}>.2)
###  add_custom_command(TARGET ${MODULE} POST_BUILD COMMAND mv $<TARGET_FILE:${MODULE}>.2 $<TARGET_FILE:${MODULE}>)
###    add_custom_command(TARGET ${MODULE} POST_BUILD COMMAND ${CROSS_COMPILE}nm ${XTENSA_CORE_OPT} $<TARGET_FILE:${MODULE}> > ${MODULE}.nm)
endforeach()

set(LIBRARY ${PROJECT_NAME}_library)
add_executable(${LIBRARY} ${CMAKE_CURRENT_LIST_DIR}/empty.c)

add_custom_command(TARGET ${LIBRARY} PRE_LINK DEPENDS ${MODULES_LIST} COMMAND ${CMAKE_COMMAND} -DMODULES_LIST="${MODULES_LIST}" -DLIBNAME=${LIBRARY} -P ${CMAKE_CURRENT_LIST_DIR}/ldscripts.cmake)

target_link_options(${LIBRARY} PRIVATE ${XTENSA_CORE_OPT})
### Maybe this is not necessary if proper linker script is provided???!!!
target_link_options(${LIBRARY} PRIVATE "-nostdlib" "-nodefaultlibs" "--verbose")
target_link_options(${LIBRARY} PRIVATE "-Wl,--no-undefined" "-Wl,--unresolved-symbols=report-all" "-Wl,--error-unresolved-symbols")
##target_link_options(${LIBRARY} PRIVATE "-Wl,--gc-sections")
target_link_options(${LIBRARY} PRIVATE "-Wl,-Map,$<TARGET_FILE:${LIBRARY}>.map")
###target_link_options(sample_module PRIVATE "-mlsp=.")
target_link_options(${LIBRARY} PRIVATE "-T" "elf32xtensa.x")

target_link_libraries(${LIBRARY} PRIVATE ${MODULES_LIST})
###target_link_libraries(${LIBRARY} PRIVATE "$<LINK_GROUP:RESCAN,${MODULES_LIST}>")
