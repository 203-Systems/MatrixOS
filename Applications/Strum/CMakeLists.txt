# Strum Application
file(GLOB_RECURSE STRUM_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE STRUM_HEADERS "*.h")

add_library(StrumAPP ${STRUM_SOURCES} ${STRUM_HEADERS})

target_include_directories(StrumAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StrumAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Strum)