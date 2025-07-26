file(GLOB_RECURSE BLEMIDI_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE BLEMIDI_HEADERS "*.h")

add_library(BLEMidi ${BLEMIDI_SOURCES} ${BLEMIDI_HEADERS})

target_include_directories(BLEMidi PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BLEMidi PUBLIC
    idf::bt
    idf::nvs_flash
    idf::console
)