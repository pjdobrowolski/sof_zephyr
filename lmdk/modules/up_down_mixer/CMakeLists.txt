set(SOF_SRC_DIR ../../../src)
set(ZEPHYR_SRC_DIR ../../../../zephyr/lib/libc/minimal/include)
target_sources(up_down_mixer PRIVATE ${SOF_SRC_DIR}/audio/up_down_mixer/up_down_mixer.c ${SOF_SRC_DIR}/audio/up_down_mixer/up_down_mixer_hifi3.c)

set_target_properties(up_down_mixer PROPERTIES
  HPSRAM_ADDR "0xa06b1000"
)

target_compile_definitions(up_down_mixer PRIVATE __ZEPHYR__=1
												 CONFIG_ZEPHYR_NATIVE_DRIVERS=1
												 CONFIG_XTENSA=1
												 CONFIG_IPC_MAJOR_4=1
												 CONFIG_LIBRARY=1
												 __ZEPHYR_RTOS_IDC_H__=1
												 MODULE_PRIVAT=1
												 XCHAL_HAVE_HIFI3=1)

												 message( ${LMDK_DIR}${SOF_SRC_DIR}../posix/include )

target_include_directories(up_down_mixer PRIVATE "${SOF_SRC_DIR}/include"
												 "${SOF_SRC_DIR}/include/coefficients"
												 "${SOF_SRC_DIR}/include/ipc"
												 "${SOF_SRC_DIR}/include/ipc4"
 												 "${SOF_SRC_DIR}/include/math"
 												 "${SOF_SRC_DIR}/include/lib"
												 "${SOF_SRC_DIR}/include/sof/audio/module_adapter/iadk"
												 "${SOF_SRC_DIR}/platform/library/include"
 												 "${SOF_SRC_DIR}/../zephyr/include"
  												 "${SOF_SRC_DIR}/../zephyr/include/zephyr/sys"
												 "${SOF_SRC_DIR}/../zephyr/lib/libc/minimal/include"
												 "${SOF_SRC_DIR}/../xtos/include")
#												 "${ZEPHYR_SRC_DIR}"
#												 "${ZEPHYR_SRC_DIR}/../../../../include"
#												 "${ZEPHYR_SRC_DIR}/../../../../include/zephyr")