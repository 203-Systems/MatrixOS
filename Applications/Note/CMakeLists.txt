# Note Application
file(GLOB_RECURSE NOTE_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE NOTE_HEADERS "*.h")

add_library(NoteAPP ${NOTE_SOURCES} ${NOTE_HEADERS})

target_include_directories(NoteAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NoteAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Note)