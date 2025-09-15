# FactoryMenu Application
file(GLOB_RECURSE FACTORYMENU_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE FACTORYMENU_HEADERS "*.h")

add_library(MystrixFactoryMenuAPP ${FACTORYMENU_SOURCES} ${FACTORYMENU_HEADERS})

target_include_directories(MystrixFactoryMenuAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MystrixFactoryMenuAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(FactoryMenu)