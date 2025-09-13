file(GLOB_RECURSE FS_HEADERS "*.h")
set(FS_SOURCES
    FS.cpp
    diskio.cpp
)

set(FATFS_DIR ${CMAKE_SOURCE_DIR}/Library/FatFs/)

# FATFS_SOURCES - exclude the template diskio.c, use our own
set(FATFS_SOURCES
    ${FATFS_DIR}/source/ff.c
)

add_library(MatrixOSFS
    ${FS_HEADERS}
    ${FS_SOURCES}
    ${FATFS_SOURCES}
)

target_include_directories(MatrixOSFS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FATFS_DIR}/source
)

target_link_libraries(MatrixOSFS PUBLIC MatrixOSInterface)