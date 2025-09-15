# Dice Application
file(GLOB_RECURSE DICE_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE DICE_HEADERS "*.h")

add_library(DiceAPP ${DICE_SOURCES} ${DICE_HEADERS})

target_include_directories(DiceAPP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(DiceAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Dice)