if(DEVICE_TINYUSB)
    file(GLOB_RECURSE HID_HEADERS "*.h")
    file(GLOB_RECURSE HID_SOURCES "*.cpp" "*.c")

    add_library(MatrixOSHID
        ${HID_HEADERS}
        ${HID_SOURCES}
    )

    target_link_libraries(MatrixOSHID PUBLIC MatrixOSInterface MatrixOSUSB)
elseif(DEVICE_CUSTOM_HID_IO)
    add_library(MatrixOSHID INTERFACE)

    target_include_directories(MatrixOSHID INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(MatrixOSHID INTERFACE MatrixOSInterface MatrixOSUSB)
else()
    # No TinyUSB: stub HID library (header-only, stubs in USBStub.cpp via MatrixOSUSB)
    add_library(MatrixOSHID
        HID.h
        HIDSpecs.h
    )

    target_link_libraries(MatrixOSHID PUBLIC MatrixOSInterface MatrixOSUSB)
endif()