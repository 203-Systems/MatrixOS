file(GLOB_RECURSE HID_HEADERS "*.h")
file(GLOB_RECURSE HID_SOURCES "*.cpp" "*.c")

add_library(MatrixOSHID
    ${HID_HEADERS}
    ${HID_SOURCES}
)

target_link_libraries(MatrixOSHID PUBLIC MatrixOSInterface MatrixOSUSB)