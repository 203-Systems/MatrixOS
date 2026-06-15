# Python Runtime for MatrixOS
# Uses MicroPython as the embedded Python interpreter.

set(MICROPYTHON_EMBED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MicroPythonEmbed)
set(MICROPYTHON_PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MicroPythonPort)

file(GLOB_RECURSE MICROPYTHON_EMBED_SOURCES
    ${MICROPYTHON_EMBED_DIR}/py/*.c
    ${MICROPYTHON_EMBED_DIR}/extmod/*.c
    ${MICROPYTHON_EMBED_DIR}/shared/runtime/*.c
    ${MICROPYTHON_EMBED_DIR}/port/*.c
)

set(MATRIXOS_MICROPYTHON_USERMOD_SOURCES
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_module.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_common.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_color.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_timer.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_sys.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_led.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_input.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_color_effects.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_nvs.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_utils.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_ui.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_ui_components.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_ui_utility.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_logging.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_filesystem.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_usb.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_hid.cpp
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos/matrixos_midi.cpp
)

add_library(Python
    Python.cpp
    Python.h
    ${MICROPYTHON_PORT_DIR}/matrixos_file.cpp
    ${MICROPYTHON_PORT_DIR}/MicroPythonRuntime.cpp
    ${MICROPYTHON_PORT_DIR}/MicroPythonRuntime.h
    ${MATRIXOS_MICROPYTHON_USERMOD_SOURCES}
    ${MICROPYTHON_EMBED_SOURCES}
)

target_include_directories(Python PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MICROPYTHON_PORT_DIR}
    ${MICROPYTHON_PORT_DIR}/usermod/matrixos
    ${MICROPYTHON_EMBED_DIR}
    ${MICROPYTHON_EMBED_DIR}/port
)

if(EMSCRIPTEN)
    target_compile_options(Python PRIVATE -pthread)
endif()

target_link_libraries(Python PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

RegisterApplicationClass(Python)
