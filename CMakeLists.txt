cmake_minimum_required(VERSION 3.16)
project(MatrixOS)

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
  message(FATAL_ERROR "Device variant ${DEVICE} found in multiple devices. Please resolve ambiguity.")
endif()

message(STATUS "DEVICE: ${DEVICE} (${FAMILY})")

set(FAMILY_PATH ${CMAKE_SOURCE_DIR}/devices/${FAMILY})
set(DEVICE_PATH ${FAMILY_PATH}/Variants/${DEVICE})

include(${FAMILY_PATH}/family.cmake)

add_executable(MatrixOS main.cpp)

# add_subdirectory(core/${MATRIX_OS_CORE})


# add_subdirectory(lib)
# add_subdirectory(${FAMILY_PATH})
add_subdirectory(os)
# add_subdirectory(applications)

# add_executable(MatrixOS main.cpp)


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


target_compile_options(MatrixOS PUBLIC
  "-DCFG_TUSB_OS=OPT_OS_FREERTOS"
)


