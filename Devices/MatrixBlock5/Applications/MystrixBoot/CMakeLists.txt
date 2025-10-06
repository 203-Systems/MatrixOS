# MystrixBoot Application
file(GLOB_RECURSE MYSTRIXBOOT_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE MYSTRIXBOOT_HEADERS "*.h")

add_library(MystrixBootAPP ${MYSTRIXBOOT_SOURCES} ${MYSTRIXBOOT_HEADERS})

target_include_directories(MystrixBootAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MystrixBootAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BootAnimationTemplate
)

# Register the application class
RegisterApplicationClass(MystrixBoot)