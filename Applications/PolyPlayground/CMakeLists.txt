# PolyPlayground Application
file(GLOB_RECURSE POLYPLAYGROUND_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE POLYPLAYGROUND_HEADERS "*.h")

add_library(PolyPlaygroundAPP ${POLYPLAYGROUND_SOURCES} ${POLYPLAYGROUND_HEADERS})

target_include_directories(PolyPlaygroundAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PolyPlaygroundAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(PolyPlayground)