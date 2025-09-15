# Reversi Application
file(GLOB_RECURSE REVERSI_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE REVERSI_HEADERS "*.h")

add_library(ReversiAPP ${REVERSI_SOURCES} ${REVERSI_HEADERS})

target_include_directories(ReversiAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ReversiAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Reversi)