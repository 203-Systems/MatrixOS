file(GLOB_RECURSE MIDI_HEADERS "*.h")
file(GLOB_RECURSE MIDI_SOURCES "*.cpp" "*.c")

add_library(MatrixOSMIDI
    ${MIDI_HEADERS}
    ${MIDI_SOURCES}
)

target_link_libraries(MatrixOSMIDI PUBLIC MatrixOSInterface)