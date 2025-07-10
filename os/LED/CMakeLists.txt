file(GLOB_RECURSE LED_HEADERS "*.h")
file(GLOB_RECURSE LED_SOURCES "*.cpp" "*.c")

add_library(MatrixOSLED
    ${LED_HEADERS}
    ${LED_SOURCES}
)

target_link_libraries(MatrixOSLED PUBLIC MatrixOSInterface)