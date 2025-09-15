# CustomControlMap Application
file(GLOB_RECURSE CUSTOMCONTROLMAP_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE CUSTOMCONTROLMAP_HEADERS "*.h")

# Add cb0r library sources
file(GLOB CB0R_SOURCES "${CMAKE_SOURCE_DIR}/Library/cb0r/src/*.c")
file(GLOB CB0R_HEADERS "${CMAKE_SOURCE_DIR}/Library/cb0r/include/*.h")
list(APPEND CUSTOMCONTROLMAP_SOURCES ${CB0R_SOURCES})
list(APPEND CUSTOMCONTROLMAP_HEADERS ${CB0R_HEADERS})

add_library(CustomControlMapAPP ${CUSTOMCONTROLMAP_SOURCES} ${CUSTOMCONTROLMAP_HEADERS})

target_include_directories(CustomControlMapAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}

    ${CMAKE_SOURCE_DIR}/Library/cb0r/include
)

target_link_libraries(CustomControlMapAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(CustomControlMap)