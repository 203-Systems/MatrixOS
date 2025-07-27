file(GLOB_RECURSE NVS_HEADERS "*.h")
file(GLOB_RECURSE NVS_SOURCES "*.cpp" "*.c")

add_library(MatrixOSNVS
    ${NVS_HEADERS}
    ${NVS_SOURCES}
)

target_include_directories(MatrixOSNVS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) 
target_link_libraries(MatrixOSNVS PUBLIC MatrixOSInterface) 