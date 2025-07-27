# BrightnessControl Application
file(GLOB_RECURSE BRIGHTNESSCONTROL_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE BRIGHTNESSCONTROL_HEADERS "*.h")

add_library(BrightnessControlUI ${BRIGHTNESSCONTROL_SOURCES} ${BRIGHTNESSCONTROL_HEADERS})

target_include_directories(BrightnessControlUI PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BrightnessControlUI PUBLIC
    MatrixOSInterface
)