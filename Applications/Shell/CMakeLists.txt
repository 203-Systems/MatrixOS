# Shell Application
file(GLOB_RECURSE SHELL_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE SHELL_HEADERS "*.h")

add_library(ShellAPP ${SHELL_SOURCES} ${SHELL_HEADERS})

target_include_directories(ShellAPP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Library/ArduinoJson/src
)

target_link_libraries(ShellAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(Shell)