set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Variants/${DEVICE})

file(GLOB_RECURSE MATRIX_OS_SRC_C
  "${CMAKE_SOURCE_DIR}/os/*.c"
)

file(GLOB_RECURSE MATRIX_OS_SRC_CPP
  "${CMAKE_SOURCE_DIR}/os/*.cpp"
)

file(GLOB_RECURSE MATRIX_OS_APPLICATION_C
  "${CMAKE_SOURCE_DIR}/applications/*.c"
  "${FAMILY_PATH}/Applications/*.cpp"
)

file(GLOB_RECURSE MATRIX_OS_APPLICATION_CPP
  "${CMAKE_SOURCE_DIR}/applications/*.cpp"
  "${FAMILY_PATH}/Applications/*.cpp"
)

file(GLOB_RECURSE DEVICE_SRC_C
  "${DEVICE_PATH}/*.c"
  "${FAMILY_PATH}/Drivers/*.c"
)

file(GLOB_RECURSE DEVICE_SRC_CPP
  "${DEVICE_PATH}/*.cpp"
  "${FAMILY_PATH}/Drivers/*.cpp"
)


set(MATRIX_OS_SRC "${CMAKE_SOURCE_DIR}/main.cpp" "${MATRIX_OS_SRC_C}" "${MATRIX_OS_SRC_CPP}")
set(DEVICE_SRC "${FAMILY_PATH}/Family.cpp" "${DEVICE_SRC_C}" "${DEVICE_SRC_CPP}")
set(MATRIX_OS_APPLICATION_SRC "${MATRIX_OS_APPLICATION_C}" "${MATRIX_OS_APPLICATION_CPP}")

idf_component_get_property( FREERTOS_ORIG_INCLUDE_PATH freertos ORIG_INCLUDE_PATH)

set(CHERRRYUSB "${CMAKE_SOURCE_DIR}/lib/CherryUSB")

set(MATRIX_OS_INC
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/core/esp32sx"
  "${CMAKE_SOURCE_DIR}/Devices"
  "${CMAKE_SOURCE_DIR}/os"
)

set(LIB_INC 
  "${CMAKE_SOURCE_DIR}/lib/printf/src"
  "${CMAKE_SOURCE_DIR}/lib/cb0r/include"
  "${CMAKE_SOURCE_DIR}/core/esp32sx/cherryusb_esp32"
  "${CHERRRYUSB}/core"
  "${CHERRRYUSB}/common"
  "${CHERRRYUSB}/class/cdc"
  "${CHERRRYUSB}/class/hid"
  "${CHERRRYUSB}/class/msc"
  "${CHERRRYUSB}/class/audio"
  "${CHERRRYUSB}/class/midi"
  "${CHERRRYUSB}/core"
)

set(LIB_SRC                             
  "${CMAKE_SOURCE_DIR}/lib/printf/src/printf/printf.c"
  "${CMAKE_SOURCE_DIR}/lib/cb0r/src/cb0r.c"
  "${CMAKE_SOURCE_DIR}/core/esp32sx/cherryusb_esp32/esp_cherryusb.c"
  "${CHERRRYUSB}/class/cdc/usbd_cdc.c"
  "${CHERRRYUSB}/class/hid/usbd_hid.c"
  "${CHERRRYUSB}/class/msc/usbd_msc.c"
  "${CHERRRYUSB}/class/audio/usbd_audio.c"
  "${CHERRRYUSB}/core/usbd_core.c"
  "${CHERRRYUSB}/port/dwc2/usb_dc_dwc2.c"
)

idf_component_register(
                    SRCS ${MATRIX_OS_SRC} ${DEVICE_SRC} ${MATRIX_OS_APPLICATION_SRC} ${LIB_SRC}
                    INCLUDE_DIRS "." ${MATRIX_OS_INC} ${FAMILY_PATH} ${DEVICE_PATH} ${FREERTOS_ORIG_INCLUDE_PATH} ${LIB_INC}
                    REQUIRES freertos nvs_flash esp_wifi esp_adc esp_wifi esp_netif efuse ulp usb)

# Apply board specific content
include("${DEVICE}/device.cmake")


# message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
# message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
# message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
# message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
# message(STATUS "Matrix OS SRC: ${CMAKE_SOURCE_DIR}/os")
# message(STATUS "Matrix OS SRC Files: ${MATRIX_OS_SRC}")
# message(STATUS "Device SRC Files: ${DEVICE_SRC}")

ulp_embed_binary(ulp_fsr_keypad "${FAMILY_PATH}/ULP/fsr_keypad.c" "${FAMILY_PATH}/Drivers/KeypadFSR.cpp")
# ulp_embed_binary(ulp_keypad "${FAMILY_PATH}/ULP/keypad.c" "${FAMILY_PATH}/Drivers/KeypadBinary.cpp")