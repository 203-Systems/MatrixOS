# Python Runtime for MatrixOS
# Uses PikaPython as the embedded Python interpreter

file(GLOB_RECURSE PYTHON_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE PYTHON_HEADERS "*.h")

# Remove problematic MatrixOS binding file
list(REMOVE_ITEM PYTHON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PikaPython/pikascript-lib/MatrixOS/MatrixOS_MatrixOS.cpp")

message(STATUS "Python sources found: ${PYTHON_SOURCES}")
message(STATUS "Python headers found: ${PYTHON_HEADERS}")

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