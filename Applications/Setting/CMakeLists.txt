# Setting Application
file(GLOB_RECURSE SETTING_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE SETTING_HEADERS "*.h")

add_library(SettingUI ${SETTING_SOURCES} ${SETTING_HEADERS})

target_include_directories(SettingUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SettingUI PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)