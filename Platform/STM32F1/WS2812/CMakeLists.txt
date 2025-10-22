file(GLOB_RECURSE WS2812_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE WS2812_HEADERS "*.h")

add_library(WS2812 ${WS2812_SOURCES} ${WS2812_HEADERS})

target_include_directories(WS2812 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/OS
)

# STM32F1 specific WS2812 configuration
target_compile_definitions(WS2812 PUBLIC
    WS2812_USE_TIMER
    WS2812_TIMER=TIM8
    WS2812_TIMER_CHANNEL=TIM_CHANNEL_2
)

target_link_libraries(WS2812 PUBLIC
    MatrixOSFramework
)