# Core layer - FreeRTOS and basic system components
add_library(MatrixOSCore INTERFACE)

target_include_directories(MatrixOSCore INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F1
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32F4
    ${CMAKE_CURRENT_SOURCE_DIR}/ESP32SX
)

# Add FreeRTOS
add_subdirectory(lib/FreeRTOS-Kernel)

# Link FreeRTOS to core
target_link_libraries(MatrixOSCore INTERFACE FreeRTOS)

# Add other core libraries
add_subdirectory(lib/printf)
add_subdirectory(lib/tinyusb)
add_subdirectory(lib/cb0r)

target_link_libraries(MatrixOSCore INTERFACE
    printf
    tinyusb
    cb0r
) 