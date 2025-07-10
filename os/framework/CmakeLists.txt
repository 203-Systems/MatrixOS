file(GLOB_RECURSE FRAMEWORK_HEADERS "*.h" "Types/*.h" "Utils/*.h" "Timer/*.h" "Storage/*.h" "Midi/*.h" "Logging/*.h" "KeyEvent/*.h" "Geometry/*.h" "Commands/*.h" "Color/*.h")
file(GLOB_RECURSE FRAMEWORK_SOURCES "*.cpp" "Timer/*.cpp" "Color/*.cpp")

add_library(MatrixOSFramework
    ${FRAMEWORK_HEADERS}
    ${FRAMEWORK_SOURCES}
)

target_include_directories(MatrixOSFramework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Types
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/Timer
    ${CMAKE_CURRENT_SOURCE_DIR}/Storage
    ${CMAKE_CURRENT_SOURCE_DIR}/Midi
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging
    ${CMAKE_CURRENT_SOURCE_DIR}/KeyEvent
    ${CMAKE_CURRENT_SOURCE_DIR}/Geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/Commands
    ${CMAKE_CURRENT_SOURCE_DIR}/Color
)

target_link_libraries(MatrixOSFramework PUBLIC MatrixOSInterface)
