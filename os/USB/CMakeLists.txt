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


add_library(MatrixOSUSB
    ${USB_HEADERS}
    ${USB_SOURCES}
    ${TINYUSB_SOURCES}
)

target_include_directories(MatrixOSUSB PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/TinyUSB
    ${TINYUSB_DIR}/src
)

target_compile_options(MatrixOSUSB PUBLIC
  "-DCFG_TUSB_MCU=OPT_MCU_${TINYUSB_MCU_OPT}"
)

target_link_libraries(MatrixOSUSB PUBLIC MatrixOSInterface) 