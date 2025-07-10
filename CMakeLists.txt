cmake_minimum_required(VERSION 3.16)

# Global C flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
  -ggdb \
  -fdata-sections \
  -ffunction-sections \
  -fsingle-precision-constant \
  -fno-strict-aliasing \
  -Wdouble-promotion \
  -Wstrict-overflow \
  -Wextra \
  -Wfloat-equal \
  -Wundef \
  -Wwrite-strings \
  -Wsign-compare \
  -Wmissing-format-attribute \
  -Wunreachable-code \
  -Wcast-align \
  -Wcast-function-type \
")

# Global C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
  -ggdb \
  -fdata-sections \
  -ffunction-sections \
  -fsingle-precision-constant \
  -fno-strict-aliasing \
  -Wdouble-promotion \
  -Wstrict-overflow \
  -Wextra \
  -Wfloat-equal \
  -Wundef \
  -Wwrite-strings \
  -Wsign-compare \
  -Wmissing-format-attribute \
  -Wunreachable-code \
  -Wcast-align \
  -Wcast-function-type \
  -std=gnu++17 \
")

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
else()
  # Extract the family name from the variant path
  list(GET VARIANT_PATHS 0 VARIANT_PATH)
  get_filename_component(VARIANTS_DIR "${VARIANT_PATH}" DIRECTORY) # .../MatrixBlock6/Variants
  get_filename_component(FAMILY_DIR "${VARIANTS_DIR}" DIRECTORY)   # .../MatrixBlock6
  get_filename_component(FAMILY "${FAMILY_DIR}" NAME)
endif()

message(STATUS "DEVICE: ${DEVICE} (${FAMILY})")

if(MODE STREQUAL "RELEASE")
    message("COMPILE AS RELEASE BUILD")
    add_compile_definitions(RELEASE_BUILD)
elseif(MODE STREQUAL "RELEASECANDIDATE")
    message("COMPILE AS RELEASE CANDIDATE BUILD")
    add_compile_definitions(RELEASE_CANDIDATE_BUILD)
elseif(MODE STREQUAL "BETA")
    message("COMPILE AS BETA BUILD")
    add_compile_definitions(BETA_BUILD)
elseif(MODE STREQUAL "NIGHTLY")
    message("COMPILE AS NIGHTY BUILD")
    add_compile_definitions(NIGHTY_BUILD)
elseif(MODE STREQUAL "DEVELOPMENT")
    message("COMPILE AS DEVELOPMENT BUILD")
    add_compile_definitions(DEVELOPMENT_BUILD)
elseif(MODE STREQUAL "UNDEFINED")
    message("COMPILE AS CODE DEFINED BUILD WITH NO OPTIMIZATIONS")
    add_compile_definitions(DEBUG_BUILD)
else()
    message( FATAL_ERROR "MODE is not defined" )
endif()

set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${FAMILY})

include(${FAMILY_PATH}/Family.cmake)

project(MatrixOS)

if(ESP_PLATFORM)
  idf_component_get_property(FREERTOS_INC freertos ORIG_INCLUDE_PATH)
endif()

add_subdirectory(Devices)
add_subdirectory(os)
add_subdirectory(${FAMILY_PATH})
add_subdirectory(Applications)


