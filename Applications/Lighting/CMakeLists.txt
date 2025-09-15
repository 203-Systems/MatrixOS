# Lighting Application
file(GLOB_RECURSE LIGHTING_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE LIGHTING_HEADERS "*.h")

add_library(LightingAPP ${LIGHTING_SOURCES} ${LIGHTING_HEADERS})

target_include_directories(LightingAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LightingAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Lighting)