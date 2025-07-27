cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32s3")

if(NOT DEFINED ENV{IDF_PATH})
  message(FATAL_ERROR " Please set up ESP-IDF environment before building.")
endif()

set(SDKCONFIG_PATH ${CMAKE_BINARY_DIR}/sdkconfig)
if(MODE STREQUAL "DEVELOPMENT")
    set(SDKCONFIG_DEFAULTS_PATH "${FAMILY_PATH}/sdkconfig.development")
else()
    set(SDKCONFIG_DEFAULTS_PATH "${FAMILY_PATH}/sdkconfig.release")
endif()

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

idf_build_process("${IDF_TARGET}"
    SDKCONFIG ${SDKCONFIG_PATH}
    SDKCONFIG_DEFAULTS ${SDKCONFIG_DEFAULTS_PATH}
    COMPONENTS esptool_py freertos usb efuse esp_common esp_system esp_driver_gpio esp_driver_uart nvs_flash esp_wifi esp_netif esp_adc ulp bt console
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

idf_build_set_property(COMPILE_OPTIONS "-Wno-maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)

idf_component_get_property(FREERTOS_INC freertos ORIG_INCLUDE_PATH)

add_library(FreeRTOS INTERFACE)
target_link_libraries(FreeRTOS INTERFACE idf::freertos)
target_include_directories(FreeRTOS INTERFACE
    ${FREERTOS_INC}
    ${FREERTOS_INC}/config/include/freertos
)

# Collect all device source files
file(GLOB_RECURSE DEVICE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/Family.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Variants/Mystrix/Device.cpp"
)

file(GLOB_RECURSE DEVICE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Variants/Mystrix/*.h"
)

add_library(MatrixOSDevice
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_include_directories(MatrixOSDevice PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Variants/Mystrix
    ${CMAKE_SOURCE_DIR}/Platform/ESP32SX
)

string(TOUPPER "${IDF_TARGET}" TINYUSB_MCU_OPT)
target_compile_options(MatrixOSDevice PUBLIC
    "-DCFG_TUSB_MCU=OPT_MCU_${TINYUSB_MCU_OPT}"
    "-DESP_PLATFORM=1"
)

# Add Platform ESP32SX libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/Platform/ESP32SX/WS2812 ${CMAKE_BINARY_DIR}/Platform/ESP32SX/WS2812)
add_subdirectory(${CMAKE_SOURCE_DIR}/Platform/ESP32SX/blemidi ${CMAKE_BINARY_DIR}/Platform/ESP32SX/blemidi)

target_link_libraries(MatrixOSDevice PUBLIC 
    DeviceInterface
    MatrixOSFramework
    WS2812
    BLEMidi
    idf::freertos 
    idf::esp_common
    idf::esp_system
    idf::usb
    idf::efuse
    idf::esp_driver_gpio
    idf::esp_driver_uart
    idf::nvs_flash
    idf::esp_wifi
    idf::esp_netif
    idf::esp_adc
    idf::ulp
)

# ulp_embed_binary(ulp_fsr_keypad "${FAMILY_PATH}/ULP/fsr_keypad.c" "${FAMILY_PATH}/Drivers/KeypadFSR.cpp")

set(elf_file ${CMAKE_PROJECT_NAME}.elf)

add_executable(${elf_file} main.c)

target_link_libraries(${elf_file}
    PRIVATE MatrixOS
)

idf_build_executable(${elf_file})