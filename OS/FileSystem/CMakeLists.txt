file(GLOB_RECURSE FILE_HEADERS "*.h")
set(FILE_SOURCES
    File.cpp
    diskio.cpp
)

set(FATFS_DIR ${CMAKE_SOURCE_DIR}/Library/FatFs/)

# FATFS_SOURCES - exclude the template diskio.c, use our own
set(FATFS_SOURCES
    ${FATFS_DIR}/source/ff.c
)

add_library(MatrixOSFile
    ${FILE_HEADERS}
    ${FILE_SOURCES}
    ${FATFS_SOURCES}
)

target_include_directories(MatrixOSFile PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FATFS_DIR}/source
)

target_link_libraries(MatrixOSFile PUBLIC MatrixOSInterface)