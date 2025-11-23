add_library(Sequencer
    Sequencer.cpp
    Sequence.cpp
    SequenceEvent.cpp
    SequenceData.cpp
    SequenceMeta.cpp
    NotePad.cpp
    ControlBar.cpp
    MixerControl.cpp
    PatternSelector.cpp
    TrackSelector.cpp
    PatternPad.cpp
    ScaleVisualizer.cpp
    ScaleModifier.cpp
    EventDetailView.cpp
    ClipLauncher.cpp
)

target_link_libraries(Sequencer PUBLIC
    MatrixOSAPPInterface
)

RegisterApplicationClass(Sequencer)
