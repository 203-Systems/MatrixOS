# Performance Application
file(GLOB_RECURSE PERFORMANCE_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE PERFORMANCE_HEADERS "*.h")

add_library(PerformanceAPP ${PERFORMANCE_SOURCES} ${PERFORMANCE_HEADERS})

target_include_directories(PerformanceAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PerformanceAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Performance)