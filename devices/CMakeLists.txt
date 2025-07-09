add_library(Device
    Device.h
)

target_link_libraries(Device
                                            PUBLIC MatrixOSFramework
                                            PUBLIC MatrixOSSystem
                                            PUBLIC MatrixOSLoggging
                                            PUBLIC DeviceFamily
)

target_include_directories(application_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
