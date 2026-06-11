# DeveloperApp Application
file(GLOB_RECURSE DEVELOPER_APP_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE DEVELOPER_APP_HEADERS "*.h")

add_library(DeveloperAppAPP ${DEVELOPER_APP_SOURCES} ${DEVELOPER_APP_HEADERS})

target_include_directories(DeveloperAppAPP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(DeveloperAppAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

RegisterApplicationClass(DeveloperApp)
