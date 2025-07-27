cmake_minimum_required(VERSION 3.10)

project(cb0r C)

file(GLOB SRC_FILES "src/*.c") # Load all files in src folder
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} OBJECT ${SRC_FILES})
