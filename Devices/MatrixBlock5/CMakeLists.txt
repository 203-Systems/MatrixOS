# MatrixBlock5 uses STM32F103
set(DEVICE_TARGET "STM32F103")

enable_language(ASM)

# Include the STM32 toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/toolchain-stm32f103.cmake)

# Common compiler flags for STM32F103
set(COMMON_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -mabi=aapcs -nostartfiles")
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu11 -Wno-unused-command-line-argument")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -fno-rtti -fno-exceptions -Wno-unused-command-line-argument")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS} -x assembler-with-cpp")

# Force Thumb mode for all compilation and assembly
# Add -w to suppress all compiler warnings as well
# Use -mthumb-interwork to improve Thumb compatibility
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb-interwork -Wa,-mthumb -Wa,-mimplicit-it=always -Wa,-mcpu=cortex-m3 -Wa,-W -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthumb-interwork -Wa,-mthumb -Wa,-mimplicit-it=always -Wa,-mcpu=cortex-m3 -Wa,-W -w")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mthumb-interwork -Wa,-mthumb -Wa,-mimplicit-it=always -Wa,-mcpu=cortex-m3 -Wa,-W -w")

# Disable assembler warnings being treated as errors globally - be more aggressive
# CMake should not fail on compiler warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error -Wno-error=all")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-error=all")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wno-error -Wno-error=all")

# Tell CMake to not fail on warnings
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)

# TinyUSB compile options for STM32F1
# These flags reduce optimization to avoid complex ARM assembly warnings
set(DEVICE_TINYUSB_COMPILE_OPTIONS
    "-O0"
    "-Wno-implicit-fallthrough"
    "-Wno-format"
    "-fno-lto"
    "-fno-fat-lto-objects"
)

# Debug flags
if(MODE STREQUAL "DEVELOPMENT" OR MODE STREQUAL "UNDEFINED")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Og")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${COMMON_FLAGS} -specs=nosys.specs -specs=nano.specs -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map -T${CMAKE_CURRENT_SOURCE_DIR}/Variants/MatrixFounderEdition/STM32F103RETx_FLASH.ld")

# Collect all device source files
file(GLOB_RECURSE DEVICE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Family.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Variants/MatrixFounderEdition/*.cpp"
    "${CMAKE_SOURCE_DIR}/Platform/STM32F1/WS2812/*.cpp"
)

# STM32 HAL source files
set(STM32_HAL_SOURCES
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_cortex.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rcc.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rcc_ex.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_dma.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_tim.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_tim_ex.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_pwr.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rtc.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rtc_ex.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_flash.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Src/stm32f1xx_hal_flash_ex.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/cmsis_device_f1/Source/Templates/system_stm32f1xx.c
)

file(GLOB_RECURSE DEVICE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Variants/MatrixFounderEdition/*.h"
)

add_library(MatrixOSDevice
    ${DEVICE_SOURCES}
    ${STM32_HAL_SOURCES}
    ${DEVICE_HEADERS}
    ${CMAKE_SOURCE_DIR}/Library/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/cmsis_device_f1/Source/Templates/gcc/startup_stm32f103xe.s
)

target_include_directories(MatrixOSDevice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Variants/MatrixFounderEdition
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1
    ${CMAKE_SOURCE_DIR}/Library/FatFs/source
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/include
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/portable/GCC/ARM_CM3
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/cmsis_device_f1/Include
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Inc
    ${CMAKE_SOURCE_DIR}/Library/CMSIS_5/CMSIS/Core/Include
    ${CMAKE_SOURCE_DIR}/Library/tinyusb/src
    ${CMAKE_SOURCE_DIR}/OS/USB/TinyUSB
)

# Add FreeRTOSConfig.h directory to compiler include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add FreeRTOS library
add_library(FreeRTOS STATIC
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/list.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/queue.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/tasks.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/timers.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/event_groups.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/stream_buffer.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
)

# Add FreeRTOS library with proper include paths
target_include_directories(FreeRTOS PUBLIC
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/include
    ${CMAKE_SOURCE_DIR}/Library/FreeRTOS-Kernel/FreeRTOS/Source/portable/GCC/ARM_CM3
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Variants/MatrixFounderEdition
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/cmsis_device_f1/Include
    ${CMAKE_SOURCE_DIR}/Platform/STM32F1/stm32f1xx_hal_driver/Inc
    ${CMAKE_SOURCE_DIR}/Library/CMSIS_5/CMSIS/Core/Include
)

# Apply Thumb mode compiler flags to FreeRTOS target
target_compile_options(FreeRTOS PUBLIC
    "-Wa,-mthumb"
    "-Wa,-mimplicit-it=thumb"
    "-Wa,-mcpu=cortex-m3"
)

# Add Thumb-2 definition for FreeRTOS
target_compile_definitions(FreeRTOS PUBLIC
    __thumb2__=1
)

# STM32F103 specific definitions
# Note: VDD_VALUE, PREFETCH_ENABLE, HSE/LSE/HSI/LSI values are defined in stm32f1xx_hal_conf.h
target_compile_definitions(MatrixOSDevice PUBLIC
    STM32F103xE
    USE_HAL_DRIVER
    CFG_TUSB_MCU=OPT_MCU_STM32F1
    __thumb2__=1
    VECT_TAB_OFFSET=0x2000
)

# Set C++ standard for MatrixOSDevice target
target_compile_features(MatrixOSDevice PUBLIC cxx_std_17)

# Link with MatrixOS framework and FreeRTOS
target_link_libraries(MatrixOSDevice PUBLIC
    DeviceInterface
    MatrixOSFramework
    FreeRTOS
)

# Create executable
set(elf_file ${CMAKE_PROJECT_NAME}.elf)

add_executable(${elf_file} ${CMAKE_SOURCE_DIR}/OS/main.cpp)
target_compile_features(${elf_file} PUBLIC cxx_std_17)

target_link_libraries(${elf_file}
    PRIVATE MatrixOS
    PRIVATE MatrixOSDevice
)

# Generate binary and hex files
add_custom_command(TARGET ${elf_file} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${elf_file} ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${elf_file} ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} ${elf_file}
    COMMENT "Generating binary and hex files"
)

# Copy ELF file to standard location for debug tools
add_custom_command(TARGET ${elf_file} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${elf_file}
        ${CMAKE_BINARY_DIR}/${elf_file}
    COMMENT "Copying ELF file to build root for debug tools"
)