# BootAnimation Application
file(GLOB_RECURSE BOOTANIMATION_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE BOOTANIMATION_HEADERS "*.h")

add_library(BootAnimationTemplate ${BOOTANIMATION_SOURCES} ${BOOTANIMATION_HEADERS})

target_include_directories(BootAnimationTemplate PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BootAnimationTemplate PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)