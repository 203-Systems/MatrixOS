add_library(matrixos_usb
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/tusb.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/common/tusb_fifo.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/device/usbd.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/device/usbd_control.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/audio/audio_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/cdc/cdc_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/dfu/dfu_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/dfu/dfu_rt_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/hid/hid_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/midi/midi_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/msc/msc_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/net/ecm_rndis_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/net/ncm_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/usbtmc/usbtmc_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/video/video_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/class/vendor/vendor_device.c
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/portable/espressif/esp32sx/dcd_esp32sx.c
  ${CMAKE_SOURCE_DIR}/lib/printf/src/printf/printf.c
  ${CMAKE_SOURCE_DIR}/lib/cb0r/src/cb0r.c
)

target_include_directories(matrixos_usb PUBLIC
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src
  ${CMAKE_SOURCE_DIR}/lib/printf/src
  ${CMAKE_SOURCE_DIR}/lib/cb0r/include
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/hw
  ${CMAKE_SOURCE_DIR}/lib/tinyusb/src
  ${CMAKE_SOURCE_DIR}/os/system/USB
)

# Set MCU define if needed (should be set globally, but can be set here as well)
if(DEFINED CFG_TUSB_MCU)
  target_compile_definitions(matrixos_usb PUBLIC CFG_TUSB_MCU=${CFG_TUSB_MCU})
endif()

# Link to MatrixOS if it exists
if(TARGET MatrixOS)
  target_link_libraries(MatrixOS PRIVATE matrixos_usb)
endif() 