# Companion Application
file(GLOB_RECURSE COMPANION_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE COMPANION_HEADERS "*.h")

add_library(CompanionAPP ${COMPANION_SOURCES} ${COMPANION_HEADERS})

target_include_directories(CompanionAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CompanionAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Companion)