add_library(DeviceInterface INTERFACE)

target_include_directories(DeviceInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(DeviceInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Device.h)
