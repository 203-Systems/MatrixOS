cmake_minimum_required(VERSION 3.16)

set(FREERTOS_INC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS)

set(MYSTRIXSIL_GIT_HASH "")
set(MYSTRIXSIL_GIT_DIRTY 0)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MYSTRIXSIL_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE MYSTRIXSIL_GIT_DIRTY_RESULT
        ERROR_QUIET
    )

    if(MYSTRIXSIL_GIT_DIRTY_RESULT EQUAL 1)
        set(MYSTRIXSIL_GIT_DIRTY 1)
    endif()
endif()

add_library(FreeRTOS STATIC
    FreeRTOS/FreeRTOSStub.cpp
)

target_include_directories(FreeRTOS PUBLIC
    ${FREERTOS_INC}
)

target_compile_features(FreeRTOS PUBLIC cxx_std_17)

add_library(MatrixOSDevice
    Device.cpp
    MatrixOSConfig.h
    Family.h
)

target_include_directories(MatrixOSDevice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(MatrixOSDevice PUBLIC
    MATRIXOS_WEB=1
    MATRIXOS_GIT_HASH="${MYSTRIXSIL_GIT_HASH}"
    MATRIXOS_GIT_DIRTY=${MYSTRIXSIL_GIT_DIRTY}
)

target_link_libraries(MatrixOSDevice PUBLIC
    DeviceInterface
    MatrixOSFramework
    FreeRTOS
)

add_executable(MatrixOSHost
    HostStub.cpp
)

target_link_libraries(MatrixOSHost PRIVATE
    MatrixOS
)

if(EMSCRIPTEN)
    target_compile_options(FreeRTOS PRIVATE -pthread)
    target_compile_options(MatrixOSDevice PRIVATE -pthread)
    target_compile_options(MatrixOSHost PRIVATE -pthread)

    target_link_options(MatrixOSHost PRIVATE
        "SHELL:-sEXPORTED_RUNTIME_METHODS=HEAPU8,UTF8ToString"
        "SHELL:-sEXPORTED_FUNCTIONS=[_main,_MatrixOS_Wasm_GetFrameBuffer,_MatrixOS_Wasm_GetFrameBufferByteLength,_MatrixOS_Wasm_GetWidth,_MatrixOS_Wasm_GetHeight,_MatrixOS_Wasm_KeyEvent,_MatrixOS_Wasm_FnEvent,_MatrixOS_Wasm_KeypadTick,_MatrixOS_Wasm_GetVersionString,_MatrixOS_Wasm_GetBuildIdentityString,_MatrixOS_Wasm_Reboot,_MatrixOS_Wasm_Bootloader,_MatrixOS_Wasm_GetRotation,_MatrixOS_Wasm_GetUptimeMs,_MatrixOS_Wasm_GetKeypadState,_MatrixOS_Wasm_GetKeypadStateLength,_MatrixOS_Wasm_GetFnState,_MatrixOS_Wasm_MidiInject,_MatrixOS_Wasm_RawHidInject,_MatrixOS_Wasm_SerialWrite,_MatrixOS_Wasm_NvsGetCount,_MatrixOS_Wasm_NvsGetHashes,_MatrixOS_Wasm_NvsGetSize,_MatrixOS_Wasm_NvsGetData,_MatrixOS_Wasm_NvsWrite,_MatrixOS_Wasm_NvsDelete,_MatrixOS_Wasm_NvsClear,_MatrixOS_Wasm_NvsExport,_MatrixOS_Wasm_NvsExportSize,_MatrixOS_Wasm_NvsImport]"
        "SHELL:-sUSE_PTHREADS=1"
        "SHELL:-sPTHREAD_POOL_SIZE=16"
        "SHELL:-sPROXY_TO_PTHREAD=1"
        "SHELL:-sALLOW_TABLE_GROWTH=1"
        -pthread
    )
endif()
