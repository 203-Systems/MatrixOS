if(DEVICE_TINYUSB)
    file(GLOB_RECURSE HID_HEADERS "*.h")
    file(GLOB_RECURSE HID_SOURCES "*.cpp" "*.c")

    add_library(MatrixOSHID
        ${HID_HEADERS}
        ${HID_SOURCES}
    )

    target_link_libraries(MatrixOSHID PUBLIC MatrixOSInterface MatrixOSUSB)
else()
    # No TinyUSB: stub HID library (header-only, stubs in USBStub.cpp via MatrixOSUSB)
    add_library(MatrixOSHID
        HID.h
        HIDSpecs.h
    )

    target_link_libraries(MatrixOSHID PUBLIC MatrixOSInterface MatrixOSUSB)
endif()