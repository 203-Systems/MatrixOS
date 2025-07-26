file(GLOB_RECURSE SYSTEM_HEADERS "*.h")
file(GLOB_RECURSE SYSTEM_SOURCES "*.cpp" "*.c")

add_library(MatrixOSSystem
    ${SYSTEM_HEADERS}
    ${SYSTEM_SOURCES}
)

target_link_libraries(MatrixOSSystem PUBLIC MatrixOSInterface MatrixOSApps)
