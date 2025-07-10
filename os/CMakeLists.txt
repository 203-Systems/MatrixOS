# OS layer - builds against device interface and framework
add_library(MatrixOSKernal INTERFACE)

target_include_directories(MatrixOSKernal INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add OS subdirectories
add_subdirectory(framework)
add_subdirectory(system)
add_subdirectory(ui)

# Link to framework, system, and UI libraries
target_link_libraries(MatrixOSKernal INTERFACE 
    MatrixOSFramework
    MatrixOSSystem
    MatrixOSUI
)

