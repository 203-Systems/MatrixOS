file(GLOB_RECURSE UI_HEADERS "*.h")
file(GLOB_RECURSE UI_SOURCES "*.cpp" "*.c")

add_library(MatrixOSUI
    ${UI_HEADERS}
    ${UI_SOURCES}
)

# target_include_directories(MatrixOSUI PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}
# )
