

file(GLOB_RECURSE MATRIX_OS_SRC_C
  "${CMAKE_SOURCE_DIR}/os/*.c"
)

file(GLOB_RECURSE MATRIX_OS_SRC_CPP
  "${CMAKE_SOURCE_DIR}/os/*.cpp"
)

set(TINYUSB "${CMAKE_SOURCE_DIR}/lib/tinyusb")

set(LIB_SRC                             
  "${TINYUSB}/src/tusb.c"
  "${TINYUSB}/src/common/tusb_fifo.c"
  "${TINYUSB}/src/device/usbd.c"
  "${TINYUSB}/src/device/usbd_control.c"
  "${TINYUSB}/src/class/audio/audio_device.c"
  "${TINYUSB}/src/class/cdc/cdc_device.c"
  "${TINYUSB}/src/class/dfu/dfu_device.c"
  "${TINYUSB}/src/class/dfu/dfu_rt_device.c"
  "${TINYUSB}/src/class/hid/hid_device.c"
  "${TINYUSB}/src/class/midi/midi_device.c"
  "${TINYUSB}/src/class/msc/msc_device.c"
  "${TINYUSB}/src/class/net/ecm_rndis_device.c"
  "${TINYUSB}/src/class/net/ncm_device.c"
  "${TINYUSB}/src/class/usbtmc/usbtmc_device.c"
  "${TINYUSB}/src/class/video/video_device.c"
  "${TINYUSB}/src/class/vendor/vendor_device.c"
  "${TINYUSB}/src/portable/espressif/esp32sx/dcd_esp32sx.c"
  "${CMAKE_SOURCE_DIR}/lib/printf/src/printf/printf.c"
  "${CMAKE_SOURCE_DIR}/lib/cb0r/src/cb0r.c"
)

set(MATRIX_OS_INC
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/core/ESP32S3"
  "${CMAKE_SOURCE_DIR}/devices"
  "${CMAKE_SOURCE_DIR}/os"
  "${CMAKE_SOURCE_DIR}/os/system/USB"
)

set(LIB_INC 
  "${CMAKE_SOURCE_DIR}/lib/tinyusb/src"
  "${CMAKE_SOURCE_DIR}/lib/printf/src"
  "${CMAKE_SOURCE_DIR}/lib/cb0r/include"
  "${TINYUSB}/hw"
  "${TINYUSB}/src"
)