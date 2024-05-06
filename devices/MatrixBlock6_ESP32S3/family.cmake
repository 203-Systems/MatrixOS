cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

# Add os and device directories
set(EXTRA_COMPONENT_DIRS "devices/MatrixBlock6_ESP32S3/variants" "core/esp32sx")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(SUPPORTED_TARGETS esp32s3)
set(FAMILY_MCUS ESP32S3)
set(IDF_TARGET "esp32s3")

set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Variants/${DEVICE})

set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.defaults)
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

add_compile_options (-fdiagnostics-color=always -Wno-maybe-uninitialized -Wno-unused-parameter -Wno-unused-but-set-parameter -Wno-narrowing)