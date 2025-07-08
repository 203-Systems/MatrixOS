cmake_minimum_required(VERSION 3.5)

# Add os and device directories
set(EXTRA_COMPONENT_DIRS "devices/MatrixBlock6/Variants" "core/ESP32SX")

# set(SUPPORTED_TARGETS esp32s3)
# set(FAMILY_MCUS ESP32S3)

set(IDF_TARGET "esp32s3")
include(${CMAKE_SOURCE_DIR}/core/ESP32SX/esp32sx.cmake)

add_compile_options (-Wno-maybe-uninitialized)
add_compile_options (-fdiagnostics-color=always)

# Clean target
add_custom_target(clean-idf
    COMMAND idf.py -B${CMAKE_BINARY_DIR} -DFAMILY=${FAMILY} -DDEVICE=${DEVICE} clean
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Cleaning the build directory"
)

# Full clean target
add_custom_target(fullclean-idf
    COMMAND idf.py -B${CMAKE_BINARY_DIR} -DFAMILY=${FAMILY} -DDEVICE=${DEVICE} fullclean
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Full clean of the build directory"
)

# Flash and related targets
foreach(target flash bootloader-flash app-flash erase monitor dfu-flash dfu)
    add_custom_target(${target}
        COMMAND idf.py -B${CMAKE_BINARY_DIR} -DFAMILY=${FAMILY} -DDEVICE=${DEVICE} ${target}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running idf.py ${target}"
    )
endforeach()

# UF2 conversion target (placeholder, assumes .bin exists)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.uf2
    COMMAND python ${CMAKE_SOURCE_DIR}/tools/uf2/utils/uf2conv.py -f 0x00000000 -b 0x0 -c -o ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.uf2 ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.bin
    DEPENDS ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.bin
    COMMENT "Converting BIN to UF2"
)
add_custom_target(uf2 DEPENDS ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.uf2)

# Upload target (placeholder, assumes uf2upload.py and .uf2 exist)
add_custom_target(uf2-upload DEPENDS uf2
    COMMAND python ${CMAKE_SOURCE_DIR}/tools/uf2/utils/uf2upload.py -f ${CMAKE_BINARY_DIR}/MatrixOS-${DEVICE}.uf2 -d "${UF2_MODEL}" -l
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Uploading OS UF2 update to Mystrix"
)

# Menuconfig target
add_custom_target(menuconfig
    COMMAND idf.py -B${CMAKE_BINARY_DIR} -DFAMILY=${FAMILY} -DDEVICE=${DEVICE} menuconfig
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running idf.py menuconfig"
)
