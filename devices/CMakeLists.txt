# Device interface library - what OS builds against
add_library(MatrixOSDeviceInterface INTERFACE)

target_include_directories(MatrixOSDeviceInterface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add device interface header
target_sources(MatrixOSDeviceInterface INTERFACE Device.h)

# Link to framework
target_link_libraries(MatrixOSDeviceInterface INTERFACE MatrixOSFramework)
