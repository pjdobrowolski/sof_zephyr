
set(IMR "0xa1608000")
set(HPSRAM "0xa06a1000")

# reserve space for manifest???
math(EXPR IMR "${IMR} + 9 * 4096" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR HPSRAM "${HPSRAM} + 9 * 4096" OUTPUT_FORMAT HEXADECIMAL)

###set(MODULES_LIST sample sample2)
#separate_arguments(MODULES_LIST NATIVE_COMMAND ${MODULES_LIST})

set(LDSCRIPT_FILE elf32xtensa.x)

file(WRITE ${LDSCRIPT_FILE} "")

file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/memory_header_linker_script.txt\n")
configure_file(${CMAKE_CURRENT_LIST_DIR}/memory_header_linker_script.txt.in ${CMAKE_CURRENT_BINARY_DIR}/memory_header_linker_script.txt)

file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/common_text_linker_script.txt\n")
file(COPY ${CMAKE_CURRENT_LIST_DIR}/common_text_linker_script.txt  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/common_rodata_linker_script.txt\n")
file(COPY ${CMAKE_CURRENT_LIST_DIR}/common_rodata_linker_script.txt  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

#foreach(MODULE ${MODULES_LIST})
#    file(READ ${MODULE}.nm NM_CONTENTS)
#    string(REGEX MATCH "[A-Za-z0-9_]+PackageEntryPoint" PACKAGE_ENTRY_POINT ${NM_CONTENTS})

    file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/text_linker_script.txt\n")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/module_text_linker_script.txt.in ${CMAKE_CURRENT_BINARY_DIR}/text_linker_script.txt)

    file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/rodata_linker_script.txt\n")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/module_rodata_linker_script.txt.in ${CMAKE_CURRENT_BINARY_DIR}/rodata_linker_script.txt)

    file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/bss_linker_script.txt\n")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/module_bss_linker_script.txt.in ${CMAKE_CURRENT_BINARY_DIR}/bss_linker_script.txt)
#endforeach()


file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/xt_linker_script.txt\n")
file(COPY ${CMAKE_CURRENT_LIST_DIR}/xt_linker_script.txt  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

###set(LIBNAME ${LIBRARY})

file(APPEND ${LDSCRIPT_FILE} "INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/guard_linker_script.txt\n")
configure_file(${CMAKE_CURRENT_LIST_DIR}/guard_linker_script.txt.in ${CMAKE_CURRENT_BINARY_DIR}/guard_linker_script.txt)
