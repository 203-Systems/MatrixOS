cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32s3")
include(${CMAKE_SOURCE_DIR}/core/ESP32SX/esp32sx.cmake)

add_compile_options (-Wno-maybe-uninitialized)
add_compile_options (-fdiagnostics-color=always)
