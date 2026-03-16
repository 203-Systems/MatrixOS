# ForceCalibration Application
file(GLOB_RECURSE FORCECALIBRATION_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE FORCECALIBRATION_HEADERS "*.h")

add_library(MystrixForceCalibrationAPP ${FORCECALIBRATION_SOURCES} ${FORCECALIBRATION_HEADERS})

target_include_directories(MystrixForceCalibrationAPP PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MystrixForceCalibrationAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
)

# Register the application class
RegisterApplicationClass(ForceCalibration)