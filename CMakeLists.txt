cmake_minimum_required(VERSION 3.16)

project(MatrixOS)

# Main executable
add_executable(MatrixOS main.cpp)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED DEVICE)
  message(FATAL_ERROR "You must specify -DDEVICE=<device>")
endif()

if(NOT DEFINED MODE)
  set(MODE UNDEFINED)
endif()
message(STATUS "Build mode: ${MODE}")
add_compile_definitions(BUILD_MODE=${MODE})

file(GLOB VARIANT_PATHS devices/*/Variants/${DEVICE})
list(LENGTH VARIANT_PATHS VARIANT_COUNT)
if(VARIANT_COUNT EQUAL 0)
  message(FATAL_ERROR "Device variant ${DEVICE} not found in any device.")
elseif(VARIANT_COUNT GREATER 1)
  set(ERROR_MESSAGE "Device variant ${DEVICE} found in multiple devices:\n")
  foreach(PATH ${VARIANT_PATHS})
    set(ERROR_MESSAGE "${ERROR_MESSAGE}  - ${PATH}\n")
  endforeach()
  set(ERROR_MESSAGE "${ERROR_MESSAGE}Please resolve ambiguity.")
  message(FATAL_ERROR "${ERROR_MESSAGE}")
endif()

message(STATUS "DEVICE: ${DEVICE} (${FAMILY})")

set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Variants/${DEVICE})

include(${FAMILY_PATH}/family.cmake)

# 4. Build OS (depends on framework and device interface)
add_subdirectory(os)

add_subdirectory(${FAMILY_PATH})

add_subdirectory(applications)


