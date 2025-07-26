cmake_minimum_required(VERSION 3.16)

project(MatrixOS-${DEVICE})

# # Global C flags
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
#   -ggdb \
#   -fdata-sections \
#   -ffunction-sections \
#   -fsingle-precision-constant \
#   -fno-strict-aliasing \
#   -Wdouble-promotion \
#   -Wstrict-overflow \
#   -Wextra \
#   -Wfloat-equal \
#   -Wundef \
#   -Wwrite-strings \
#   -Wsign-compare \
#   -Wmissing-format-attribute \
#   -Wunreachable-code \
#   -Wcast-align \
#   -Wcast-function-type \
# ")

# # Global C++ flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
#   -ggdb \
#   -fdata-sections \
#   -ffunction-sections \
#   -fsingle-precision-constant \
#   -fno-strict-aliasing \
#   -Wdouble-promotion \
#   -Wstrict-overflow \
#   -Wextra \
#   -Wfloat-equal \
#   -Wundef \
#   -Wwrite-strings \
#   -Wsign-compare \
#   -Wmissing-format-attribute \
#   -Wunreachable-code \
#   -Wcast-align \
#   -Wcast-function-type \
#   -std=gnu++17 \
# ")

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED DEVICE)
  message(FATAL_ERROR "You must specify -DDEVICE=<device>")
endif()

if(NOT DEFINED FAMILY)
  set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${DEVICE})
else()
  set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/Devices/${FAMILY})
endif()

message(STATUS "DEVICE: ${DEVICE} ${FAMILY} (${FAMILY_PATH})")

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

add_subdirectory(Devices)
add_subdirectory(os)
add_subdirectory(${FAMILY_PATH})
# add_subdirectory(Applications)
