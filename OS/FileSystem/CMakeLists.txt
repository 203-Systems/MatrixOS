
# Only compile FileSystem.cpp if device has storage
if(DEVICE_STORAGE)
    file(GLOB_RECURSE FILE_HEADERS "*.h")

    # Local FatFS sources - only needed when storage is available
    set(FILE_SOURCES
        FileSystem.cpp
        File.cpp
        FatFS/ff.c
        FatFS/ffunicode.c
        FatFS/diskio.cpp
        FatFS/ffsystem.c
    )
else()
    set(FILE_HEADERS "")
    set(FILE_SOURCES "")
endif()

add_library(MatrixOSFile
    ${FILE_HEADERS}
    ${FILE_SOURCES}
)

target_include_directories(MatrixOSFile PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FatFS
)

target_link_libraries(MatrixOSFile PUBLIC MatrixOSInterface MatrixOSAPPInterface)