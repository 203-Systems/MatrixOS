add_library(Sequencer
    Sequencer.cpp
    SequenceEvent.cpp
)

target_link_libraries(Sequencer PUBLIC
    MatrixOSAPPInterface
)

RegisterApplicationClass(Sequencer)