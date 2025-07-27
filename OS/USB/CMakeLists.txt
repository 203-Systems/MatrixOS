file(GLOB_RECURSE USB_HEADERS "*.h")
file(GLOB_RECURSE USB_SOURCES "*.cpp" "*.c")

set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/Library/tinyusb/)

# TINYUSB_SOURCES
set(TINYUSB_SOURCES
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/class/audio/audio_device.c
    ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
    ${TINYUSB_DIR}/src/class/dfu/dfu_device.c
    ${TINYUSB_DIR}/src/class/dfu/dfu_rt_device.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/midi/midi_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/class/net/ecm_rndis_device.c
    ${TINYUSB_DIR}/src/class/net/ncm_device.c
    ${TINYUSB_DIR}/src/class/usbtmc/usbtmc_device.c
    ${TINYUSB_DIR}/src/class/video/video_device.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
)

# if(TINYUSB_MCU_OPT STREQUAL "ESP32S3" OR TINYUSB_MCU_OPT STREQUAL "ESP32S2")
    list(APPEND TINYUSB_SOURCES "${CMAKE_SOURCE_DIR}/Library/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c" )
# endif()

set(PRINTF_DIR ${CMAKE_SOURCE_DIR}/Library/printf/)

set(PRINTF_SOURCES
    ${PRINTF_DIR}/src/printf/printf.c
)

add_library(MatrixOSUSB
    ${USB_HEADERS}
    ${USB_SOURCES}
    ${TINYUSB_SOURCES}
    ${PRINTF_SOURCES}
)

target_include_directories(MatrixOSUSB PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/TinyUSB
    ${TINYUSB_DIR}/src
    ${PRINTF_DIR}/src
    ${PRINTF_DIR}/src/printf
)

# target_compile_options(MatrixOSUSB PUBLIC
#     "-DCFG_TUSB_MCU=OPT_MCU_${TINYUSB_MCU_OPT}"  
# )

target_link_libraries(MatrixOSUSB PUBLIC MatrixOSInterface) 