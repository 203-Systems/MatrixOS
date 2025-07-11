if(NOT DEFINED ENV{IDF_PATH})
  message(FATAL_ERROR " Please set up ESP-IDF environment before building.")
endif()

message(STATUS "IDF_PATH: $ENV{IDF_PATH}")

set(PROJECT_NAME "MatrixOS-${DEVICE}")
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

if(MODE STREQUAL "DEVELOPMENT")
    set(SDKCONFIG_PATH "${FAMILY_PATH}/sdkconfig.development")
else()
    set(SDKCONFIG_PATH "${FAMILY_PATH}/sdkconfig.release")
endif()

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

idf_build_process("${IDF_TARGET}"
    COMPONENTS freertos  esptool_py
    SDKCONFIG ${SDKCONFIG_PATH}
    BUILD_DIR ${CMAKE_BINARY_DIR}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(elf_file ${CMAKE_PROJECT_NAME}.elf)

idf_build_set_property(COMPILE_OPTIONS "-Wno-maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)

idf_component_get_property(FREERTOS_INC freertos ORIG_INCLUDE_PATH)
string(TOUPPER "${IDF_TARGET}" TINYUSB_MCU_OPT)