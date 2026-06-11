# Developer Application
file(GLOB_RECURSE DEVELOPER_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE DEVELOPER_HEADERS "*.h")

add_library(Developer ${DEVELOPER_SOURCES} ${DEVELOPER_HEADERS})

target_include_directories(Developer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Developer PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

RegisterApplicationClass(Developer)
