set(component_srcs "ws2812.cpp")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/src"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"
                       REQUIRES "")

