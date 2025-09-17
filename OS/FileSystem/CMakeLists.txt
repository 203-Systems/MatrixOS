file(GLOB_RECURSE FILE_HEADERS "*.h")
set(FILE_SOURCES
    File.cpp
    FileSystem.cpp
)

# Local FatFS sources
set(FATFS_SOURCES
    FatFS/ff.c
    FatFS/ffunicode.c
    FatFS/diskio.cpp
    FatFS/ffsystem.c
)

add_library(MatrixOSFile
    ${FILE_HEADERS}
    ${FILE_SOURCES}
    ${FATFS_SOURCES}
)

target_include_directories(MatrixOSFile PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FatFS
)

target_link_libraries(MatrixOSFile PUBLIC MatrixOSInterface MatrixOSAPPInterface)