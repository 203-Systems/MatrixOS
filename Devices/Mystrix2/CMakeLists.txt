cmake_minimum_required(VERSION 3.16)

# Required ESP-IDF version
set(REQUIRED_IDF_VERSION_MAJOR 5)
set(REQUIRED_IDF_VERSION_MINOR 3)

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
    COMPONENTS esptool_py freertos usb efuse esp_common esp_system esp_driver_gpio esp_driver_uart nvs_flash esp_wifi esp_netif esp_adc ulp bt console sdmmc driver
)

# Check ESP-IDF version
if(NOT (IDF_VERSION_MAJOR EQUAL ${REQUIRED_IDF_VERSION_MAJOR} AND IDF_VERSION_MINOR EQUAL ${REQUIRED_IDF_VERSION_MINOR}))
  string(ASCII 27 ESC)
  message(FATAL_ERROR "${ESC}[31mESP-IDF version ${REQUIRED_IDF_VERSION_MAJOR}.${REQUIRED_IDF_VERSION_MINOR}.x is required. Current version: ${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}${ESC}[0m")
else()
  string(ASCII 27 ESC)
  message(STATUS "${ESC}[32mUsing ESP-IDF version ${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}${ESC}[0m")
endif()

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
    ${CMAKE_SOURCE_DIR}/Library/FatFs/source
)

string(TOUPPER "${IDF_TARGET}" TINYUSB_MCU_OPT)
target_compile_options(MatrixOSDevice PUBLIC
    "-DCFG_TUSB_MCU=OPT_MCU_${TINYUSB_MCU_OPT}"
    "-DESP_PLATFORM=1"
)

# Add Platform ESP32SX libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/Platform/ESP32SX/WS2812 ${CMAKE_BINARY_DIR}/Platform/ESP32SX/WS2812)
add_subdirectory(${CMAKE_SOURCE_DIR}/Platform/ESP32SX/BLEMidi ${CMAKE_BINARY_DIR}/Platform/ESP32SX/BLEMidi)

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
    idf::sdmmc
    idf::driver
)

enable_language(ASM)
set(python $ENV{IDF_PYTHON_ENV_PATH})
set(COMPONENT_LIB MatrixOSDevice)
set(COMPONENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPONENT_TARGET MatrixOSDevice)
ulp_embed_binary(ulp_fsr_keypad "${FAMILY_PATH}/ULP/fsr_keypad.c" "${FAMILY_PATH}/Drivers/KeypadFSR.cpp")

set(elf_file ${CMAKE_PROJECT_NAME}.elf)

add_executable(${elf_file} main.c)

target_link_libraries(${elf_file}
    PRIVATE MatrixOS
)

idf_build_executable(${elf_file})

# Copy ELF file to standard location for monitor tools
add_custom_command(TARGET ${elf_file} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${elf_file}
        ${CMAKE_BINARY_DIR}/${elf_file}
    COMMENT "Copying ELF file to build root for monitor tools"
)

# Workaround to enable idf.py commands
set(PROJECT_EXECUTABLE ${elf_file})
set(PROJECT_PATH ${CMAKE_SOURCE_DIR})
set(PROJECT_BUILD_TYPE "flash_app")
set(common_component_reqs_json "[]")
set(build_components_json "[]")
set(build_component_paths_json "[]")
set(build_component_info_json "{}")
set(all_component_info_json "{}")
set(debug_prefix_map_gdbinit "")

configure_file("${idf_path}/tools/cmake/project_description.json.in"
"${build_dir}/project_description.json.templ")
file(READ "${build_dir}/project_description.json.templ" project_description_json_templ)
file(REMOVE "${build_dir}/project_description.json.templ")
file(GENERATE OUTPUT "${build_dir}/project_description.json"
 CONTENT "${project_description_json_templ}")