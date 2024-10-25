cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

# Add os and device directories
set(EXTRA_COMPONENT_DIRS "devices/MatrixBlock6/Variants" "core/ESP32S3")
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(SUPPORTED_TARGETS esp32s3)
set(FAMILY_MCUS ESP32S3)
set(IDF_TARGET "esp32s3")

set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Variants/${DEVICE})


message ("Mode: ${MODE}")

if(MODE STREQUAL "RELEASE")
    message("COMPILE AS RELEASE BUILD")
    add_compile_definitions(RELEASE_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.release)
elseif(MODE STREQUAL "RELEASE CANDIDATE")
    message("COMPILE AS RELEASE CANDIDATE BUILD")
    add_compile_definitions(RELEASE_CANDIDATE_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.release)
elseif(MODE STREQUAL "BETA")
    message("COMPILE AS BETA BUILD")
    add_compile_definitions(BETA_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.release)
elseif(MODE STREQUAL "NIGHTY")
    message("COMPILE AS NIGHTY BUILD")
    add_compile_definitions(NIGHTY_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.release)
elseif(MODE STREQUAL "DEVELOPMENT")
    message("COMPILE AS DEVELOPMENT BUILD")
    add_compile_definitions(DEVELOPMENT_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.development)
elseif(MODE STREQUAL "UNDEFINED")
    message("COMPILE AS CODE DEFINED BUILD WITH NO OPTIMIZATIONS")
    add_compile_definitions(DEBUG_BUILD)
    set(SDKCONFIG_DEFAULTS ${FAMILY_PATH}/sdkconfig.development)
endif()

add_compile_options (-Wno-maybe-uninitialized)
add_compile_options (-fdiagnostics-color=always)
