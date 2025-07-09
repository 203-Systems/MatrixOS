set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_component_get_property( FREERTOS_ORIG_INCLUDE_PATH freertos ORIG_INCLUDE_PATH)

string(TOUPPER "${IDF_TARGET}" IDF_TARGET_UPPER)

target_compile_options(MatrixOS PUBLIC
  "-DCFG_TUSB_MCU=OPT_MCU_${IDF_TARGET_UPPER}"
)
