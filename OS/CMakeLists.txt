# Add all subdirectories
add_subdirectory(HID)
add_subdirectory(KeyPad)
add_subdirectory(LED)
add_subdirectory(Logging)
add_subdirectory(MIDI)
add_subdirectory(System)
add_subdirectory(NVS)
add_subdirectory(UI)
add_subdirectory(Framework)
add_subdirectory(USB)
add_subdirectory(FileSystem)

add_library(MatrixOSInterface INTERFACE)
target_include_directories(MatrixOSInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_INC})
target_sources(MatrixOSInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/MatrixOS.h)
target_link_libraries(MatrixOSInterface INTERFACE FreeRTOS MatrixOSFramework DeviceInterface MatrixOSDevice)

# MatrixOS umbrella target with main.cpp
add_library(MatrixOS main.cpp
)
target_link_libraries(MatrixOS PUBLIC
    MatrixOSInterface
    MatrixOSFramework
    MatrixOSSystem
    MatrixOSLogging
    MatrixOSKeyPad
    MatrixOSLED
    MatrixOSNVS
    MatrixOSUSB
    MatrixOSUI
    MatrixOSHID
    MatrixOSMIDI
    MatrixOSFile
)