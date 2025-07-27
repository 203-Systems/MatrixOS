file(GLOB_RECURSE LOGGING_HEADERS "*.h")
file(GLOB_RECURSE LOGGING_SOURCES "*.cpp" "*.c")

add_library(MatrixOSLogging
    ${LOGGING_HEADERS}
    ${LOGGING_SOURCES}
)

target_link_libraries(MatrixOSLogging PUBLIC MatrixOSInterface)