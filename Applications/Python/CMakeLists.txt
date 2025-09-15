# Python Runtime for MatrixOS
# Uses PikaPython as the embedded Python interpreter

file(GLOB_RECURSE PYTHON_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE PYTHON_HEADERS "*.h")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/PikaPython/pikascript-api
    ${CMAKE_CURRENT_SOURCE_DIR}/PikaPython/pikascript-core
    ${CMAKE_CURRENT_SOURCE_DIR}/PikaPython/pikascript-lib
)

add_library(Python ${PYTHON_SOURCES} ${PYTHON_HEADERS})

target_link_libraries(Python PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(Python)