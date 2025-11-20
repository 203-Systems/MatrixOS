add_library(Sequencer
    Sequencer.cpp
    Sequence.cpp
    SequenceEvent.cpp
    NotePad.cpp
)

target_link_libraries(Sequencer PUBLIC
    MatrixOSAPPInterface
)

RegisterApplicationClass(Sequencer)