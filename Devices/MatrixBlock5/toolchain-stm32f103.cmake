# STM32F103 CMake Toolchain File

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Use Raspberry Pi Pico SDK toolchain
set(STM32_TOOLCHAIN_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/gcc-arm-none-eabi")

if(WIN32)
    set(TOOLCHAIN_PREFIX "${STM32_TOOLCHAIN_PATH}/bin/arm-none-eabi-")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++.exe")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc.exe")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy.exe")
    set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size.exe")
else()
    set(TOOLCHAIN_PREFIX "${STM32_TOOLCHAIN_PATH}/bin/arm-none-eabi-")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy")
    set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -mfpu=auto")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -mfpu=auto -fno-rtti -fno-exceptions")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -mfpu=auto -x assembler-with-cpp")

# Suppress ARM assembler warnings - be more aggressive
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -Wa,--no-warn -Wno-error -Wno-error=all -Wno-unused-command-line-argument -Wno-implicit-fallthrough -Wno-format")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -Wa,--no-warn -Wno-error -Wno-error=all -Wno-unused-command-line-argument -Wno-implicit-fallthrough -Wno-format")
set(CMAKE_ASM_FLAGS_INIT "${CMAKE_ASM_FLAGS_INIT} -Wa,--no-warn -Wno-error -Wno-error=all -Wno-unused-command-line-argument -Wno-implicit-fallthrough -Wno-format")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=nosys.specs -specs=nano.specs -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)