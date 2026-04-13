if(DEVICE_CUSTOM_MIDI_IO)
    add_library(MatrixOSMIDI INTERFACE)

    target_include_directories(MatrixOSMIDI INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(MatrixOSMIDI INTERFACE MatrixOSInterface)
else()
    file(GLOB_RECURSE MIDI_HEADERS "*.h")
    file(GLOB_RECURSE MIDI_SOURCES "*.cpp" "*.c")

    add_library(MatrixOSMIDI
        ${MIDI_HEADERS}
        ${MIDI_SOURCES}
    )

    target_link_libraries(MatrixOSMIDI PUBLIC MatrixOSInterface)
endif()