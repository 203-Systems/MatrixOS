add_subdirectory(framework)
add_subdirectory(system)
add_subdirectory(ui)

add_library(MatrixOS
    MatrixOS.h
)


target_link_libraries(MatrixOS
                                            PUBLIC MatrixOSFramework
                                            PUBLIC MatrixOSSystem
                                            PUBLIC MatrixOSUI
)

