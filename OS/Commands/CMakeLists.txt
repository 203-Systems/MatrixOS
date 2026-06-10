file(GLOB_RECURSE COMMANDS_HEADERS "*.h")
file(GLOB_RECURSE COMMANDS_SOURCES "*.cpp" "*.c")

add_library(MatrixOSCommands
    ${COMMANDS_HEADERS}
    ${COMMANDS_SOURCES}
)

target_include_directories(MatrixOSCommands PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MatrixOSCommands PUBLIC MatrixOSInterface)
