set(component_srcs "WS2812.cpp")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/os" #Uses Matrix OS Framework
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"
                       REQUIRES "")

