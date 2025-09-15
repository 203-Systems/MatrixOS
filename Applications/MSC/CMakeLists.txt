add_library(MSC
    MSC.h
    MSC.cpp
)

target_include_directories(MSC PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MSC PUBLIC
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(MSC)