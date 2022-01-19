set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Varients/${DEVICE})

file(GLOB_RECURSE MATRIX_OS_SRC_C
  "${CMAKE_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE MATRIX_OS_SRC_CPP
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE DEVICE_SRC_C
  "${DEVICE_PATH}/*.c"
  "${FAMILY_PATH}/Drivers/*.c"
)

file(GLOB_RECURSE DEVICE_SRC_CPP
  "${DEVICE_PATH}/*.cpp"
  "${FAMILY_PATH}/Drivers/*.cpp"
)


set(MATRIX_OS_SRC "${MATRIX_OS_SRC_C};${MATRIX_OS_SRC_CPP}")
set(DEVICE_SRC "${DEVICE_SRC_C};${DEVICE_SRC_CPP}")


idf_component_register(SRCS "${FAMILY_PATH}/Family.cpp" "${CMAKE_SOURCE_DIR}/main.cpp" ${MATRIX_OS_SRC} ${DEVICE_SRC}
                    INCLUDE_DIRS "." "${DEVICE_PATH}" "${FAMILY_PATH}"
                    PRIV_REQUIRES "driver"
                    REQUIRES freertos nvs_flash)


# Apply board specific content
include("${DEVICE}/device.cmake")

idf_component_get_property( FREERTOS_ORIG_INCLUDE_PATH freertos ORIG_INCLUDE_PATH)
target_include_directories(${COMPONENT_TARGET} PUBLIC
  "${FREERTOS_ORIG_INCLUDE_PATH}"
)

# message(" ")
# message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
# message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
# message("PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
# message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
# message("Matrix OS SRC: ${CMAKE_SOURCE_DIR}/src")
# message(" ")
# message("Matrix OS SRC Files: ${MATRIX_OS_SRC}")
# message(" ")
# message("Device SRC Files: ${DEVICE_SRC}")
# message(" ")

# add_executable(MatrixOS ${MATRIX_OS_SRC})

set(TINYUSB "${CMAKE_SOURCE_DIR}/lib/tinyusb")
# message(TINYUSB: ${TINYUSB})

target_include_directories(${COMPONENT_TARGET} PUBLIC
  "${CMAKE_SOURCE_DIR}/core/esp32s3"
  "${CMAKE_SOURCE_DIR}/Devices"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/usb"
  "${CMAKE_SOURCE_DIR}/lib/tinyusb/src"
  "${CMAKE_SOURCE_DIR}/lib/printf"
  "${TINYUSB}/hw"
  "${TINYUSB}/src"
)

target_sources(${COMPONENT_TARGET} PUBLIC
"${TINYUSB}/src/tusb.c"
"${TINYUSB}/src/common/tusb_fifo.c"
"${TINYUSB}/src/device/usbd.c"
"${TINYUSB}/src/device/usbd_control.c"
"${TINYUSB}/src/class/audio/audio_device.c"
"${TINYUSB}/src/class/cdc/cdc_device.c"
"${TINYUSB}/src/class/dfu/dfu_device.c"
"${TINYUSB}/src/class/dfu/dfu_rt_device.c"
"${TINYUSB}/src/class/hid/hid_device.c"
"${TINYUSB}/src/class/midi/midi_device.c"
"${TINYUSB}/src/class/msc/msc_device.c"
"${TINYUSB}/src/class/net/ecm_rndis_device.c"
"${TINYUSB}/src/class/net/ncm_device.c"
"${TINYUSB}/src/class/usbtmc/usbtmc_device.c"
"${TINYUSB}/src/class/video/video_device.c"
"${TINYUSB}/src/class/vendor/vendor_device.c"
"${TINYUSB}/src/portable/espressif/esp32sx/dcd_esp32sx.c"
)

# set_source_files_properties(
#   "${FAMILY_PATH}/Family.cpp" "${CMAKE_SOURCE_DIR}/main.cpp" ${MATRIX_OS_SRC_CPP} ${DEVICE_SRC_CPP}
#     PROPERTIES COMPILE_FLAGS
#     -std=c++11
# )