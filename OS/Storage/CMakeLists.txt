file(GLOB_RECURSE STORAGE_HEADERS "*.h")
set(STORAGE_SOURCES
    StorageMutex.cpp
)

add_library(MatrixOSStorage
    ${STORAGE_HEADERS}
    ${STORAGE_SOURCES}
)

target_include_directories(MatrixOSStorage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MatrixOSStorage PUBLIC MatrixOSInterface)