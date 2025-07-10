# Framework layer - depends on core
add_library(MatrixOSFramework INTERFACE)

target_include_directories(MatrixOSFramework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add framework source files
file(GLOB_RECURSE FRAMEWORK_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE FRAMEWORK_HEADERS "*.h")

target_sources(MatrixOSFramework INTERFACE ${FRAMEWORK_SOURCES} ${FRAMEWORK_HEADERS})

# Link to core
target_link_libraries(MatrixOSFramework INTERFACE MatrixOSCore) 