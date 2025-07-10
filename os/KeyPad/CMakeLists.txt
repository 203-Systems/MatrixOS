file(GLOB_RECURSE KEYPAD_HEADERS "*.h")
file(GLOB_RECURSE KEYPAD_SOURCES "*.cpp" "*.c")

add_library(MatrixOSKeyPad
    ${KEYPAD_HEADERS}
    ${KEYPAD_SOURCES}
)

target_link_libraries(MatrixOSKeyPad PUBLIC MatrixOSInterface)