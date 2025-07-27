file(GLOB_RECURSE UI_HEADERS "*.h" "Data/*.h" "Component/*.h")
file(GLOB_RECURSE UI_SOURCES "*.cpp" "Utilities/*.cpp")

add_library(MatrixOSUI
    ${UI_HEADERS}
    ${UI_SOURCES}
)

target_include_directories(MatrixOSUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Data
    ${CMAKE_CURRENT_SOURCE_DIR}/Component
)

target_link_libraries(MatrixOSUI PUBLIC MatrixOSInterface)
