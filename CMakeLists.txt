cmake_minimum_required(VERSION 3.5)

# use BOARD-Directory name for project id
get_filename_component(PROJECT ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(PROJECT "MatrixOS-${DEVICE}")

# Check for Device info
message("Family: ${FAMILY}")
if(IDF_TARGET MATCHES "^esp32s[2-3]")
  include(devices/${FAMILY}/family.cmake)
  project(${PROJECT})
else()
  message(FATAL_ERROR "Invalid target specified: ${IDF_TARGET}")
endif()
