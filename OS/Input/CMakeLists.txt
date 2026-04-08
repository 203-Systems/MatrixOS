file(GLOB_RECURSE INPUT_HEADERS "*.h")
file(GLOB_RECURSE INPUT_SOURCES "*.cpp" "*.c")

add_library(MatrixOSInput
    ${INPUT_HEADERS}
    ${INPUT_SOURCES}
)

target_link_libraries(MatrixOSInput PUBLIC MatrixOSInterface)
