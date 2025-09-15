# REDACTED Application
file(GLOB_RECURSE REDACTED_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE REDACTED_HEADERS "*.h")

add_library(REDACTEDAPP ${REDACTED_SOURCES} ${REDACTED_HEADERS})

target_include_directories(REDACTEDAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(REDACTEDAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(REDACTED)