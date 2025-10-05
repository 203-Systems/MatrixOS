# STM32F103 CMake Toolchain File

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Assume toolchain is in PATH like MatrixOS-2.6.0
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

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