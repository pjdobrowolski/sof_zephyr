function(MODULE_SIGN MOD_NAME TOML_DIR RIMAGE_BIN_DIR SIGNING_KEY)
    if(NOT DEFINED SIGNING_KEY)
    message(FATAL_ERROR
        " Please define SIGNING_KEY: path to signing key for rimage.\n"
        " E.g. using cmake -DSIGNING_KEY=/path/to/key.pem command line parameter."
    )
    endif()

    if(NOT DEFINED SIGNING_KEY)
    message(FATAL_ERROR
        " Please define RIMAGE_BIN_DIR: path to rimage binary file.\n"
        " E.g. using cmake -DRIMAGE_BIN_DIR=/path/to/rimage.{bin/exe} command line parameter."
    )
    endif()

    cmake_parse_arguments(MODULE_SIGN "${MOD_NAME}" "${TOML_DIR}" "${RIMAGE_BIN_DIR}" "${SIGNING_KEY}" ${ARGN})
    set(RIMAGE_OUTPUT_FILE ${MOD_NAME}_noextmft)
    set(OUTPUT_FILE ${MOD_NAME}.bin)

    add_custom_target(${MOD_NAME}_target ALL
        DEPENDS ${MODULES_LIST}
        COMMAND ${RIMAGE_BIN_DIR} -k ${SIGNING_KEY} -f 2.0.0 -b 1 -o ${RIMAGE_OUTPUT_FILE} -c ${TOML_DIR} -e ${MOD_NAME}
        COMMAND ${CMAKE_COMMAND} -E cat ${RIMAGE_OUTPUT_FILE}.xman ${RIMAGE_OUTPUT_FILE} > ${OUTPUT_FILE}
    )
endfunction()