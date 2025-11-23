#include "Sequencer.h"
#include "UI/UI.h"

#include "Scales.h"
#include "PatternSelector.h"
#include "TrackSelector.h"
#include "ClipLauncher.h"
#include "MixerControl.h"
#include "PatternPad.h"
#include "NotePad.h"
#include "ControlBar.h"
#include "ScaleVisualizer.h"
#include "ScaleModifier.h"
#include "ScaleSelector.h"
#include "EventDetailView.h"

void Sequencer::Setup(const vector<string> &args)
{
    if (!Load(saveSlot))
    {
        saveSlot = 0;
        sequence.New(8);
        meta.New(8);
        Save(saveSlot);
    }

    sequence.EnableClockOutput(meta.clockOutput);

    SequencerUI();
}

void Sequencer::SequencerUI()
{
    UI sequencerUI("SequencerUI", Color(0x00FFFF), false);

    uint8_t track = this->track;

    SequencePattern *pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, sequence.GetPosition(track).pattern);

    PatternPad patternPad(this, &this->stepSelected, &this->noteSelected, &this->noteActive);
    patternPad.SetEnableFunc([&]() -> bool
                                     { return currentView == ViewMode::Sequencer; });
    sequencerUI.AddUIComponent(patternPad, Point(0, 1));

    SequencerNotePad notePad(this);
    notePad.OnSelect([&](bool noteOn, uint8_t note, uint8_t velocity) -> void
                     {
                         if (pattern != nullptr && noteOn)
                         {
                             bool existAlready = false;

                             for (const auto &step : stepSelected)
                             {
                                 uint16_t pulsesPerStep = sequence.GetPulsesPerStep();
                                 uint16_t startTime = step * pulsesPerStep;
                                 uint16_t endTime = startTime + pulsesPerStep - 1;

                                 if (pattern->RemoveNoteEventsInRange(startTime, endTime, note))
                                 {
                                     existAlready = true;
                                     noteActive.erase(noteActive.find(note));
                                 }
                             }

                             if (existAlready == false)
                             {
                                 SequenceEvent event = SequenceEvent::Note(note, velocity, false);
                                 for (const auto &step : stepSelected)
                                 {
                                      pattern->AddEvent(step * sequence.GetPulsesPerStep(), event);
                                     noteActive.insert(note);
                                 }
                             }
                         }

                         // TODO: Pass into Sequence to record
                     });
    notePad.SetEnableFunc([&]() -> bool
                          { return currentView == ViewMode::Sequencer; });
    sequencerUI.AddUIComponent(notePad, Point(0, 3));

    PatternSelector patternSelector(this);
    patternSelector.OnChange([&](uint8_t patternIdx) -> void
                             {
        ClearActiveNotes();
        pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, patternIdx); });
    patternSelector.SetEnableFunc([&]() -> bool
                                  { 
                                    bool enable = currentView == ViewMode::Sequencer && ((ShiftActive() && ((MatrixOS::SYS::Millis() - shiftOnTime) > 100)) || patternView); 
                                    patternViewActive = enable;
                                    return enable; 
                                });
    sequencerUI.AddUIComponent(patternSelector, Point(0, 3));

    // Session View
    ClipLauncher clipLauncher(this);
    clipLauncher.OnChange([&](uint8_t track, uint8_t clip) -> void
                          {
        stepSelected.clear();

        ClearActiveNotes();
        ClearSelectedNotes();

        notePad.GenerateKeymap();

        pattern = &sequence.GetPattern(track, clip, sequence.GetPosition(track).pattern); });
    clipLauncher.SetEnableFunc([&]() -> bool
                               { return currentView == ViewMode::Session; });
    sequencerUI.AddUIComponent(clipLauncher, Point(0, 0));

    // Mixer View
    MixerControl mixerControl(this);
    mixerControl.SetEnableFunc([&]() -> bool
                               { return currentView == ViewMode::Mixer; });
    sequencerUI.AddUIComponent(mixerControl, Point(0, 0));

    // Step Detail View
    EventDetailView eventDetailView(this);
    eventDetailView.SetEnableFunc([&]() -> bool
                                  { return currentView == ViewMode::StepDetail; });
    sequencerUI.AddUIComponent(eventDetailView, Point(0, 0));

    // Global
    TrackSelector trackSelector(this);
    trackSelector.OnChange([&](uint8_t val) -> void
                           {
        stepSelected.clear();

        ClearActiveNotes();
        ClearSelectedNotes();

        notePad.GenerateKeymap();

        track = this->track;

        pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, sequence.GetPosition(track).pattern); });
    trackSelector.SetEnableFunc([&]() -> bool
                                { return currentView != ViewMode::Session && currentView != ViewMode::Mixer && currentView != ViewMode::StepDetail; });
    sequencerUI.AddUIComponent(trackSelector, Point(0, 0));

    ControlBar controlBar(this, &notePad);
    controlBar.OnClear([&]() -> void
                       {
        for (const auto& step : stepSelected)
        {
            uint16_t pulsesPerStep = sequence.GetPulsesPerStep();
            pattern->ClearStepEvents(step, pulsesPerStep);
        } });
    sequencerUI.AddUIComponent(controlBar, Point(0, 7));

    sequencerUI.SetGlobalLoopFunc([&]() -> void
                                  { sequence.Tick(); });

    sequencerUI.AllowExit(false);
    sequencerUI.SetKeyEventHandler([&](KeyEvent *keyEvent) -> bool
                                   {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == PRESSED)
      {
        if(currentView != ViewMode::Sequencer)
        {
            currentView = ViewMode::Sequencer;
        }
        else
        {
            // Clean up the state
            clear = false;
            copy = false;
            shift[0] = shift[1] = false;
            shiftEventOccured[0] = shiftEventOccured[1] = false;

            ClearActiveNotes();
            ClearSelectedNotes();

            SequencerMenu();

            notePad.GenerateKeymap();
        }
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false; });

    sequencerUI.Start();
}

void Sequencer::SequencerMenu()
{
    UI sequencerMenu("Sequencer Menu", Color(0x00FFFF), true);

    TrackSelector trackSelector(this, true);
    sequencerMenu.AddUIComponent(trackSelector, Point(0, 0));

    // Right side, Track specific settings
    UIButton trackColorSelectorBtn;
    trackColorSelectorBtn.SetName("Track Color Selector");
    trackColorSelectorBtn.SetColorFunc([&]() -> Color
                                  {
        if(track < meta.tracks.size()) {
            return meta.tracks[track].color;
        }
        return Color::White; });
    trackColorSelectorBtn.OnPress([&]() -> void
                             {
        if(track < meta.tracks.size() && MatrixOS::UIUtility::ColorPicker(meta.tracks[track].color, false)) {
            sequence.SetDirty();
        } });
    sequencerMenu.AddUIComponent(trackColorSelectorBtn, Point(7, 2));

    UIButton layoutSelectorBtn;
    layoutSelectorBtn.SetName("Layout Selector");
    layoutSelectorBtn.SetColor(Color(0xFFFF00));
    layoutSelectorBtn.OnPress([&]() -> void
                              { LayoutSelector(); });
    sequencerMenu.AddUIComponent(layoutSelectorBtn, Point(7, 3));

    UIButton channelSelectorBtn;
    channelSelectorBtn.SetName("Channel Selector");
    channelSelectorBtn.SetColor(Color(0x60FF00));
    channelSelectorBtn.OnPress([&]() -> void
                               { ChannelSelector(); });
    sequencerMenu.AddUIComponent(channelSelectorBtn, Point(7, 4));

    UIButton forceSensitiveToggle;
    forceSensitiveToggle.SetName("Velocity Sensitive");
    if (Device::KeyPad::velocity_sensitivity)
    {
        forceSensitiveToggle.SetColorFunc([&]() -> Color
                                          { return Color(0x00FFB0).DimIfNot(meta.tracks[track].velocitySensitive); });
        forceSensitiveToggle.OnPress([&]() -> void
                                     { meta.tracks[track].velocitySensitive = !meta.tracks[track].velocitySensitive; sequence.SetDirty(); });
        forceSensitiveToggle.OnHold([&]() -> void
                                    { MatrixOS::UIUtility::TextScroll(forceSensitiveToggle.GetName() + " " + (meta.tracks[track].velocitySensitive ? "On" : "Off"), forceSensitiveToggle.GetColor()); });
    }
    else
    {
        forceSensitiveToggle.SetColor(Color(0x00FFB0).Dim());
        forceSensitiveToggle.OnHold([&]() -> void
                                    { MatrixOS::UIUtility::TextScroll("Velocity Sensitivity Not Supported", Color(0x00FFB0)); });
    }
    sequencerMenu.AddUIComponent(forceSensitiveToggle, Point(7, 5));

    // Left side, Sequencer Global settings
    UIButton sequenceColorSelectorBtn;
    sequenceColorSelectorBtn.SetName("Project Color Selector");
    sequenceColorSelectorBtn.SetColorFunc([&]() -> Color
        {
            return meta.color;
        });
    sequenceColorSelectorBtn.OnPress([&]() -> void
                             {
        if(MatrixOS::UIUtility::ColorPicker(meta.color, false)) {
            sequence.SetDirty();
        } });
    sequencerMenu.AddUIComponent(sequenceColorSelectorBtn, Point(0, 2));

    UIButton bpmSelectorBtn;
    bpmSelectorBtn.SetName("BPM Selector");
    bpmSelectorBtn.SetColor(Color(0xFF0080));
    bpmSelectorBtn.OnPress([&]() -> void
                           { BPMSelector(); });
    sequencerMenu.AddUIComponent(bpmSelectorBtn, Point(0, 3));

    UIButton swingSelectorBtn;
    swingSelectorBtn.SetName("Swing Selector");
    swingSelectorBtn.SetColor(Color(0xFFA000));
    swingSelectorBtn.OnPress([&]() -> void
                             { SwingSelector(); });
    sequencerMenu.AddUIComponent(swingSelectorBtn, Point(0, 4));

    UIButton barLengthSelectorBtn;
    barLengthSelectorBtn.SetName("Bar Length Selector");
    barLengthSelectorBtn.SetColor(Color(0x00A0FF));
    barLengthSelectorBtn.OnPress([&]() -> void
                                 { BarLengthSelector(); });
    sequencerMenu.AddUIComponent(barLengthSelectorBtn, Point(0, 5));

    UIButton saveBtn;
    saveBtn.SetName("Save Sequence");
    saveBtn.SetColorFunc([&]() -> Color
                         { 
                            return sequence.GetDirty() ? 
                            ColorEffects::ColorBreathLowBound(Color::Red) : 
                            meta.color; 
                        });
    saveBtn.SetSize(Dimension(2, 2));
    saveBtn.OnPress([&]() -> void
                    {
            if(Save(saveSlot))
            {
                sequence.SetDirty(false);
            }
            });
    sequencerMenu.AddUIComponent(saveBtn, Point(3, 3));

    UIButton sequenceBrowserBtn;
    sequenceBrowserBtn.SetName("Sequence Browser");
    sequenceBrowserBtn.SetColorFunc([&]() -> Color
                                    { return ColorEffects::Rainbow(3000); });
    sequenceBrowserBtn.SetSize(Dimension(4, 1));
    sequenceBrowserBtn.OnPress([&]() -> void
                               { 
                                // SequenceBrowser(); 
                            });
    sequencerMenu.AddUIComponent(sequenceBrowserBtn, Point(2, 7));

    UIButton systemSettingBtn;
    systemSettingBtn.SetName("System Setting");
    systemSettingBtn.SetColor(Color::White);
    systemSettingBtn.OnPress([&]() -> void
                             { MatrixOS::SYS::OpenSetting(); });
    sequencerMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

    sequencerMenu.AllowExit(false);
    sequencerMenu.SetKeyEventHandler([&](KeyEvent *keyEvent) -> bool
                                     {
        if (keyEvent->id == FUNCTION_KEY)
        {
            if (keyEvent->info.state == HOLD)
            {
                Exit();
            }
            else if (keyEvent->info.state == RELEASED)
            {
                sequencerMenu.Exit();
            }
            return true;
        }
        return false; });

    sequencerMenu.Start();
}

void Sequencer::LayoutSelector()
{
    Color noteColor = Color(0xFFFF00);
    Color drumColor = Color(0xFF8000);
    Color ccColor = Color(0x0080FF);
    Color pcColor = Color(0x00FF80);
    
    UI layoutSelector("Layout Selector", noteColor, false);

    TrackSelector trackSelector(this, true);
    layoutSelector.AddUIComponent(trackSelector, Point(0, 0));

    UIButton scaleModeBtn;
    scaleModeBtn.SetName("Scale Mode");
    scaleModeBtn.SetColorFunc([&]() -> Color {
        return noteColor.DimIfNot(
            meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
            meta.tracks[track].config.note.type == SequenceNoteType::Scale
        );
    });
    scaleModeBtn.OnPress([&]() -> void {
        meta.tracks[track].mode = SequenceTrackMode::NoteTrack;
        meta.tracks[track].config.note.type = SequenceNoteType::Scale;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(scaleModeBtn, Point(0, 1));

    UIButton chromaticModeBtn;
    chromaticModeBtn.SetName("Chromatic Mode");
    chromaticModeBtn.SetColorFunc([&]() -> Color {
        return noteColor.DimIfNot(
            meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
            meta.tracks[track].config.note.type == SequenceNoteType::Chromatic
        );
    });
    chromaticModeBtn.OnPress([&]() -> void {
        meta.tracks[track].mode = SequenceTrackMode::NoteTrack;
        meta.tracks[track].config.note.type = SequenceNoteType::Chromatic;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(chromaticModeBtn, Point(1, 1));

    UIButton pianoModeBtn;
    pianoModeBtn.SetName("Piano Mode");
    pianoModeBtn.SetColorFunc([&]() -> Color {
        return noteColor.DimIfNot(
            meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
            meta.tracks[track].config.note.type == SequenceNoteType::Piano
        );
    });
    pianoModeBtn.OnPress([&]() -> void {
        meta.tracks[track].mode = SequenceTrackMode::NoteTrack;
        meta.tracks[track].config.note.type = SequenceNoteType::Piano;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(pianoModeBtn, Point(2, 1));

    UIButton drumModeBtn;
    drumModeBtn.SetName("Drum Mode");
    drumModeBtn.SetColorFunc([&]() -> Color {
        return drumColor.DimIfNot(meta.tracks[track].mode == SequenceTrackMode::DrumTrack);
    });
    drumModeBtn.OnPress([&]() -> void {
        meta.tracks[track].mode = SequenceTrackMode::DrumTrack;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(drumModeBtn, Point(3, 1));

    // Enforce Scale Toggle
    UIButton enforceScaleBtn;
    enforceScaleBtn.SetName("Enforce Scale");
    enforceScaleBtn.SetColorFunc([&]() -> Color {
        return Color(0xFFFFFF).DimIfNot(meta.tracks[track].config.note.enforceScale);
    });
    enforceScaleBtn.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               (meta.tracks[track].config.note.type == SequenceNoteType::Scale ||
                meta.tracks[track].config.note.type == SequenceNoteType::Chromatic);
    });
    enforceScaleBtn.OnPress([&]() -> void {
        meta.tracks[track].config.note.enforceScale = !meta.tracks[track].config.note.enforceScale;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(enforceScaleBtn, Point(1, 2));

    // Custom Scale Toggle
    UIButton customScaleEnableBtn;
    customScaleEnableBtn.SetName("Custom Scale");
    customScaleEnableBtn.SetColorFunc([&]() -> Color {
        return Color(0x00FFFF).DimIfNot(meta.tracks[track].config.note.customScale);
    });
    customScaleEnableBtn.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack;
    });
    customScaleEnableBtn.OnPress([&]() -> void {
        if (!meta.tracks[track].config.note.customScale) {
            // Enable custom scale - initialize to default major scale
            meta.tracks[track].config.note.scale = (uint16_t)Scale::MAJOR;
            meta.tracks[track].config.note.customScale = true;
            sequence.SetDirty();
        } else {
            // Disable custom scale - reset to minor
            meta.tracks[track].config.note.scale = (uint16_t)Scale::MINOR;
            meta.tracks[track].config.note.customScale = false;
            sequence.SetDirty();
        }
    });
    layoutSelector.AddUIComponent(customScaleEnableBtn, Point(0, 2));

    ScaleVisualizer scaleVisualizer(Color(0x8000FF), Color(0xFF0080),  Color(0xFF0080));
    scaleVisualizer.SetGetRootKeyFunc([&]() -> uint8_t { return meta.tracks[track].config.note.root; });
    scaleVisualizer.SetGetRootOffsetFunc([&]() -> uint8_t { return meta.tracks[track].config.note.rootOffset; });
    scaleVisualizer.SetGetScaleFunc([&]() -> uint16_t { return meta.tracks[track].config.note.scale; });
    scaleVisualizer.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack;
    });
    scaleVisualizer.OnChange([&](uint8_t root, uint8_t offset, uint16_t /*scale*/) -> void {
        meta.tracks[track].config.note.root = root;
        meta.tracks[track].config.note.rootOffset = offset;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(scaleVisualizer, Point(0, 4));

    UIButton offsetModeBtn;
    offsetModeBtn.SetName("Modern Diatonic");
    offsetModeBtn.SetColorFunc([&]() -> Color {
        return scaleVisualizer.offsetMode ? Color(0xFF0080) : Color(0x8000FF);
    });
    offsetModeBtn.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack;
    });
    offsetModeBtn.OnPress([&]() -> void { scaleVisualizer.offsetMode = !scaleVisualizer.offsetMode; });
    layoutSelector.AddUIComponent(offsetModeBtn, Point(7, 5));

    // Scale Selector
    ScaleSelector scaleSelectorBar(Color(0xFF0090));
    scaleSelectorBar.SetScaleFunc([&]() -> uint16_t { return meta.tracks[track].config.note.scale; });
    scaleSelectorBar.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               !meta.tracks[track].config.note.customScale;
    });
    scaleSelectorBar.OnChange([&](uint16_t scale) -> void {
        sequence.SetDirty();
        meta.tracks[track].config.note.rootOffset = 0;
        meta.tracks[track].config.note.customScale = false;
        meta.tracks[track].config.note.scale = scale;
    });
    layoutSelector.AddUIComponent(scaleSelectorBar, Point(0, 6));

    // Custom Scale Modifier (remains same position for editing)
    SequenceScaleModifier scaleModifier(Color(0x00FFFF), Color(0x0040FF));
    scaleModifier.SetScaleFunc([&]() -> uint16_t { return meta.tracks[track].config.note.scale; });
    scaleModifier.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               meta.tracks[track].config.note.customScale;
    });
    scaleModifier.OnChange([&](uint16_t newScale) -> void {
        meta.tracks[track].config.note.scale = newScale;
        meta.tracks[track].config.note.customScale = true;
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(scaleModifier, Point(0, 6));

    // Octave display
    UITimedDisplay octTextDisplay(500);
    octTextDisplay.SetDimension(Dimension(8, 4));
    octTextDisplay.SetRenderFunc([&](Point origin) -> void {
        // O
        MatrixOS::LED::SetColor(origin + Point(0, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 3), noteColor);

        // C
        MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

        // T
        MatrixOS::LED::SetColor(origin + Point(5, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 0), noteColor);
    });
    octTextDisplay.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               meta.tracks[track].config.note.type == SequenceNoteType::Scale;
    });
    layoutSelector.AddUIComponent(octTextDisplay, Point(0, 4));

    // Chromatic display
    UITimedDisplay chmTextDisplay(500);
    chmTextDisplay.SetDimension(Dimension(8, 4));
    chmTextDisplay.SetRenderFunc([&](Point origin) -> void {
        // C
        MatrixOS::LED::SetColor(origin + Point(0, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 3), noteColor);

        // H
        MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

        // M
        MatrixOS::LED::SetColor(origin + Point(5, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 3), noteColor);
    });
    chmTextDisplay.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               meta.tracks[track].config.note.type == SequenceNoteType::Chromatic;
    });
    layoutSelector.AddUIComponent(chmTextDisplay, Point(0, 4));

    // Piano display
    UITimedDisplay pioTextDisplay(500);
    pioTextDisplay.SetDimension(Dimension(8, 4));
    pioTextDisplay.SetRenderFunc([&](Point origin) -> void {
        // P
        MatrixOS::LED::SetColor(origin + Point(0, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(1, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(2, 2), noteColor);

        // I
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);


        // O
        MatrixOS::LED::SetColor(origin + Point(5, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(5, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(6, 3), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 0), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 1), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 2), noteColor);
        MatrixOS::LED::SetColor(origin + Point(7, 3), noteColor);
    });
    pioTextDisplay.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack &&
               meta.tracks[track].config.note.type == SequenceNoteType::Piano;
    });
    layoutSelector.AddUIComponent(pioTextDisplay, Point(0, 4));

    // Drum display
    UITimedDisplay drmTextDisplay(UINT32_MAX);
    drmTextDisplay.SetDimension(Dimension(8, 4));
    drmTextDisplay.SetRenderFunc([&](Point origin) -> void {
        // D
        MatrixOS::LED::SetColor(origin + Point(0, 0), drumColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), drumColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), drumColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), drumColor);
        MatrixOS::LED::SetColor(origin + Point(1, 0), drumColor);
        MatrixOS::LED::SetColor(origin + Point(1, 3), drumColor);
        MatrixOS::LED::SetColor(origin + Point(2, 1), drumColor);
        MatrixOS::LED::SetColor(origin + Point(2, 2), drumColor);

        // r
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);


        // m
        MatrixOS::LED::SetColor(origin + Point(5, 1), drumColor);
        MatrixOS::LED::SetColor(origin + Point(5, 2), drumColor);
        MatrixOS::LED::SetColor(origin + Point(5, 3), drumColor);
        MatrixOS::LED::SetColor(origin + Point(6, 1), drumColor);
        MatrixOS::LED::SetColor(origin + Point(6, 2), drumColor);
        MatrixOS::LED::SetColor(origin + Point(7, 1), drumColor);
        MatrixOS::LED::SetColor(origin + Point(7, 2), drumColor);
        MatrixOS::LED::SetColor(origin + Point(7, 3), drumColor);
    });
    drmTextDisplay.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::DrumTrack;
    });
    layoutSelector.AddUIComponent(drmTextDisplay, Point(0, 4));

    layoutSelector.Start();
}

void Sequencer::ChannelSelector()
{
    Color color = Color(0x60FF00);
    uint8_t track = this->track;
    UI channelSelector("Channel Selector", color, false);
    uint16_t channel = sequence.GetChannel(track);
    int32_t offsettedChannel = channel + 1;

    TrackSelector trackSelector(this);
    trackSelector.OnChange([&](uint8_t val) -> void
                           {
        if (sequence.GetChannel(track) != channel) {
            sequence.SetChannel(track, channel);
        }

        track = this->track;
        channel = sequence.GetChannel(track);
        offsettedChannel = channel + 1; });
    channelSelector.AddUIComponent(trackSelector, Point(0, 0));

    UI4pxNumber numDisplay;
    numDisplay.SetColor(color);
    numDisplay.SetDigits(2);
    numDisplay.SetValuePointer(&offsettedChannel);
    numDisplay.SetAlternativeColor(Color::White);
    numDisplay.SetSpacing(1);
    channelSelector.AddUIComponent(numDisplay, Point(1, 1));

    UISelector channelInput;
    channelInput.SetDimension(Dimension(8, 2));
    channelInput.SetName("Channel");
    channelInput.SetColor(color);
    channelInput.SetCount(16);
    channelInput.SetValuePointer((uint16_t *)&channel);
    channelInput.OnChange([&](uint16_t val) -> void
                          { offsettedChannel = val + 1; });

    channelSelector.AddUIComponent(channelInput, Point(0, 6));

    channelSelector.SetPostRenderFunc([&]() -> void
                                      {
    // C
    MatrixOS::LED::SetColor(Point(0, 1), color);
    MatrixOS::LED::SetColor(Point(0, 2), color);
    MatrixOS::LED::SetColor(Point(0, 3), color);
    MatrixOS::LED::SetColor(Point(0, 4), color);
    MatrixOS::LED::SetColor(Point(1, 1), color);
    MatrixOS::LED::SetColor(Point(1, 4), color);

    if(channel < 9)
    {
      //h
      MatrixOS::LED::SetColor(Point(2, 1), Color::White);
      MatrixOS::LED::SetColor(Point(2, 2), Color::White);
      MatrixOS::LED::SetColor(Point(2, 3), Color::White);
      MatrixOS::LED::SetColor(Point(2, 4), Color::White);
      MatrixOS::LED::SetColor(Point(3, 2), Color::White);
      MatrixOS::LED::SetColor(Point(4, 1), Color::White);
      MatrixOS::LED::SetColor(Point(4, 2), Color::White);
      MatrixOS::LED::SetColor(Point(4, 3), Color::White);
      MatrixOS::LED::SetColor(Point(4, 4), Color::White);
    } });

    channelSelector.Start();

    if (sequence.GetChannel(track) != channel)
    {
        sequence.SetChannel(track, channel);
    }
}

void Sequencer::BPMSelector()
{
    Color color = 0xFF0080;
    UI bpmUI("BPM Selector", color, false);

    int32_t bpmValue = sequence.GetBPM();

    // BPM text display
    UITimedDisplay bpmTextDisplay(500);
    bpmTextDisplay.SetDimension(Dimension(8, 4));
    bpmTextDisplay.SetRenderFunc([&](Point origin) -> void
                                 {
    // B
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // P
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);

    // M
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color); });
    bpmUI.AddUIComponent(bpmTextDisplay, Point(0, 0));

    UI4pxNumber bpmDisplay;
    bpmDisplay.SetColor(color);
    bpmDisplay.SetDigits(3);
    bpmDisplay.SetValuePointer(&bpmValue);
    bpmDisplay.SetAlternativeColor(Color::White);
    bpmDisplay.SetEnableFunc([&]() -> bool
                             { return !bpmTextDisplay.IsEnabled(); });
    bpmUI.AddUIComponent(bpmDisplay, Point(-1, 0));

    const int32_t coarseModifier[8] = {-25, -10, -5, -1, 1, 5, 10, 25};
    const uint8_t modifierGradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};
  
    UINumberModifier bpmNumberModifier;
    bpmNumberModifier.SetColor(color);
    bpmNumberModifier.SetLength(8);
    bpmNumberModifier.SetValuePointer(&bpmValue);
    bpmNumberModifier.SetModifiers(coarseModifier);
    bpmNumberModifier.SetControlGradient(modifierGradient);
    bpmNumberModifier.SetLowerLimit(20);
    bpmNumberModifier.SetUpperLimit(299);
    bpmNumberModifier.OnChange([&](int32_t val) -> void
    {
        bpmValue = val;
        sequence.SetBPM((uint16_t)val);
        bpmTextDisplay.Disable();
    });
    bpmUI.AddUIComponent(bpmNumberModifier, Point(0, 7));

    UIToggle clockOutputToggle;
    clockOutputToggle.SetName("Clock Output");
    clockOutputToggle.SetColor(Color(0x80FF00));
    clockOutputToggle.SetValuePointer(&meta.clockOutput);
    clockOutputToggle.OnPress([&]() -> void
                              { sequence.EnableClockOutput(meta.clockOutput); sequence.SetDirty(); });
    bpmUI.AddUIComponent(clockOutputToggle, Point(0, 6));

    UIButton resetButton;
    resetButton.SetName("Reset BPM");
    resetButton.SetColor(Color::Red);
    resetButton.OnPress([&]() -> void
    {
        bpmValue = 120;
        sequence.SetBPM(120);
        bpmTextDisplay.Disable();
    });
    bpmUI.AddUIComponent(resetButton, Point(7, 6));

    bpmUI.Start();

    if (sequence.GetBPM() != (uint16_t)bpmValue)
    {
        sequence.SetBPM((uint16_t)bpmValue);
    }
}

void Sequencer::SwingSelector()
{
    Color color = 0xFFA000;
    UI swingUI("Swing Selector", color, false);

    int32_t swingValue = sequence.GetSwing();

    // Swing text display
    UITimedDisplay swingTextDisplay(500);
    swingTextDisplay.SetDimension(Dimension(8, 4));
    swingTextDisplay.SetRenderFunc([&](Point origin) -> void
                                   {
    // S
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // W
    MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // G
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
});
    swingUI.AddUIComponent(swingTextDisplay, Point(0, 0));

    UI4pxNumber swingDisplay;
    swingDisplay.SetColor(color);
    swingDisplay.SetDigits(3);
    swingDisplay.SetValuePointer(&swingValue);
    swingDisplay.SetAlternativeColor(Color::White);
    swingDisplay.SetEnableFunc([&]() -> bool
                               { return !swingTextDisplay.IsEnabled(); });
    swingUI.AddUIComponent(swingDisplay, Point(-1, 0));

    const int32_t fineModifier[8] {-10, -5, -2, -1, 1, 2, 5, 10};
    const uint8_t modifierGradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};

    UINumberModifier swingNumberModifier;
    swingNumberModifier.SetColor(color);
    swingNumberModifier.SetLength(8);
    swingNumberModifier.SetValuePointer(&swingValue);
    swingNumberModifier.SetModifiers(fineModifier);
    swingNumberModifier.SetControlGradient(modifierGradient);
    swingNumberModifier.SetLowerLimit(20);
    swingNumberModifier.SetUpperLimit(80);
    swingNumberModifier.OnChange([&](int32_t val) -> void
    {
        swingValue = val;
        sequence.SetSwing((uint8_t)val);
        swingTextDisplay.Disable();
    });
    swingUI.AddUIComponent(swingNumberModifier, Point(0, 7));

    UIButton resetButton;
    resetButton.SetName("Reset Swing");
    resetButton.SetColor(Color::Red);
    resetButton.OnPress([&]() -> void
    {
        swingValue = 50;
        sequence.SetSwing(50);
        swingTextDisplay.Disable();
    });
    swingUI.AddUIComponent(resetButton, Point(7, 6));

    swingUI.Start();

    if (sequence.GetSwing() != (uint8_t)swingValue)
    {
        sequence.SetSwing((uint8_t)swingValue);
    }
}

void Sequencer::BarLengthSelector()
{
    Color color = Color(0x00A0FF);
    Color updateColor = Color(0x80FF00);
    uint32_t updateTime = 0;
    UI barLengthSelector("Bar Length Selector", color, false);
    int32_t barLength = sequence.GetBarLength();
    int32_t offsettedBarLength = barLength - 1;

    // Bar Length text display
    UITimedDisplay barLengthTextDisplay(500);
    barLengthTextDisplay.SetDimension(Dimension(8, 4));
    barLengthTextDisplay.SetRenderFunc([&](Point origin) -> void
                                       {
        // L
        MatrixOS::LED::SetColor(origin + Point(0, 0), color);
        MatrixOS::LED::SetColor(origin + Point(0, 1), color);
        MatrixOS::LED::SetColor(origin + Point(0, 2), color);
        MatrixOS::LED::SetColor(origin + Point(0, 3), color);
        MatrixOS::LED::SetColor(origin + Point(1, 3), color);

        // E
        MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

        // n
        MatrixOS::LED::SetColor(origin + Point(5, 1), color);
        MatrixOS::LED::SetColor(origin + Point(5, 2), color);
        MatrixOS::LED::SetColor(origin + Point(5, 3), color);
        MatrixOS::LED::SetColor(origin + Point(6, 1), color);
        MatrixOS::LED::SetColor(origin + Point(7, 2), color);
        MatrixOS::LED::SetColor(origin + Point(7, 3), color);
    });
    barLengthSelector.AddUIComponent(barLengthTextDisplay, Point(0, 0));

    UI4pxNumber numDisplay;
    numDisplay.SetColorFunc([&](uint16_t digit) -> Color
    {
        if(digit % 2 == 0)
        {
            return (MatrixOS::SYS::Millis() - updateTime < 1000) ?  updateColor : color;
        }
        else
        {
            return Color::White;
        }
    });
    numDisplay.SetDigits(2);
    numDisplay.SetValuePointer(&barLength);
    numDisplay.SetSpacing(1);
    numDisplay.SetEnableFunc([&]() -> bool
                             { return !barLengthTextDisplay.IsEnabled(); });
    barLengthSelector.AddUIComponent(numDisplay, Point(1, 0));

    UISelector barLengthInput;
    barLengthInput.SetDimension(Dimension(8, 2));
    barLengthInput.SetName("Bar Length");
    barLengthInput.SetColorFunc([&]() -> Color
                           {return (MatrixOS::SYS::Millis() - updateTime < 1000) ?  updateColor : color;});
    barLengthInput.SetCount(64);
    barLengthInput.SetValuePointer((uint16_t *)&offsettedBarLength);
    barLengthInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
    barLengthInput.OnChange([&](uint16_t val) -> void
                            {
                                barLength = val + 1;
                                barLengthTextDisplay.Disable();
                                updateTime = 0;
                            });

    barLengthSelector.AddUIComponent(barLengthInput, Point(0, 6));

    UIButton updatePatternsBtn;
    updatePatternsBtn.SetName("Update Empty Patterns");
    updatePatternsBtn.SetColor(updateColor);
    updatePatternsBtn.OnPress([&]() -> void {
        sequence.SetBarLength(barLength);
        sequence.UpdateEmptyPatternsWithBarLength();
        updateTime = MatrixOS::SYS::Millis();
    });
    barLengthSelector.AddUIComponent(updatePatternsBtn, Point(7, 5));

    barLengthSelector.Start();

    if (sequence.GetBarLength() != barLength)
    {
        sequence.SetBarLength(barLength);
    }
}

bool Sequencer::ClearActive()
{
    return clear;
}

bool Sequencer::CopyActive()
{
    return copy;
}

bool Sequencer::ShiftActive()
{
    return shift[0] || shift[1];
}

void Sequencer::ShiftEventOccured()
{
    if (shift[0]) { shiftEventOccured[0] = true; }
    else if (shift[1]) { shiftEventOccured[1] = true; }
    shiftOnTime = MatrixOS::SYS::Millis();
}

void Sequencer::ClearActiveNotes()
{
    uint8_t channel = sequence.GetChannel(track);
    for (const auto &note : noteActive)
    {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
    }
    noteActive.clear();
}

void Sequencer::ClearSelectedNotes()
{
    uint8_t channel = sequence.GetChannel(track);
    for (const auto &[note, velocity] : noteSelected)
    {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
    }
    noteSelected.clear();
}

bool Sequencer::IsNoteActive(uint8_t note) const
{
    return noteActive.count(note) > 0;
}

void Sequencer::SetView(ViewMode view)
{
    if (currentView == view)
    {
        return;
    }

    if (currentView == ViewMode::Sequencer)
    {
        ClearSelectedNotes();
        ClearActiveNotes();
    }

    currentView = view;
}

bool Sequencer::Save(uint16_t slot)
{
    saveSlotType type = static_cast<saveSlotType>(slot >> 10);
    uint16_t idx = slot & 0x03FF;
    if (type == saveSlotType::OnBoard)
    {
        MLOGD("Sequencer", "Save on-board slot %u", idx);
        return SaveOnBoard(idx);
    }
    else if (type == saveSlotType::SDCard)
    {
        MLOGD("Sequencer", "Save SD slot %u", idx);
        return SaveSD(idx);
    }
    MLOGD("Sequencer", "Save failed: unknown slot type %u", static_cast<uint16_t>(type));
    return false;
}

bool Sequencer::SaveOnBoard(uint16_t slot)
{
    if (slot >= ONBOARD_SLOT_MAX) { return false; }
    uint16_t encodedSlot = (static_cast<uint16_t>(saveSlotType::OnBoard) << 10) | slot;
    saveSlot = encodedSlot;
    std::vector<uint8_t> seqBuf;
    std::vector<uint8_t> metaBuf;
    if (!SerializeSequenceData(sequence.GetData(), seqBuf)) { MLOGE("Sequencer", "SerializeSequenceData failed"); return false; }
    if (!SerializeSequenceMeta(meta, metaBuf)) { MLOGE("Sequencer", "SerializeSequenceMeta failed"); return false; }
    MLOGD("Sequencer", "SaveOnBoard slot %u sizes seq=%u meta=%u", slot, (unsigned)seqBuf.size(), (unsigned)metaBuf.size());
    if (!MatrixOS::NVS::SetVariable(SEQUENCER_DATA_HASH, seqBuf.data(), seqBuf.size()))
    {
        MLOGE("Sequencer", "SaveOnBoard failed writing sequence data");
        return false;
    }
    if (!MatrixOS::NVS::SetVariable(SEQUENCER_META_HASH, metaBuf.data(), metaBuf.size()))
    {
        MLOGE("Sequencer", "SaveOnBoard failed writing sequence meta");
        return false;
    }
    MLOGD("Sequencer", "Sequence saved to on board slot %d", slot);
    return true;
}

bool Sequencer::SaveSD(uint16_t slot)
{
    if (slot >= SD_SLOT_MAX) { MLOGD("Sequencer", "SaveSD slot out of range %u", slot); return false; }
    // TODO Implmentation
    return false;
}

bool Sequencer::Load(uint16_t slot)
{
    saveSlotType type = static_cast<saveSlotType>(slot >> 10);
    uint16_t idx = slot & 0x03FF;

    if (type == saveSlotType::OnBoard)
    {
        MLOGD("Sequencer", "Load on-board slot %u", idx);
        return LoadOnBoard(idx);
    }
    else if (type == saveSlotType::SDCard)
    {
        MLOGD("Sequencer", "Load SD slot %u", idx);
        return LoadSD(idx);
    }
    MLOGD("Sequencer", "Load failed: unknown slot type %u", static_cast<uint16_t>(type));
    return false;
}

bool Sequencer::LoadOnBoard(uint16_t slot)
{
    if (slot >= ONBOARD_SLOT_MAX) { MLOGD("Sequencer", "LoadOnBoard slot out of range %u", slot); return false; }
    size_t seqSize = MatrixOS::NVS::GetSize(SEQUENCER_DATA_HASH);
    size_t metaSize = MatrixOS::NVS::GetSize(SEQUENCER_META_HASH);
    constexpr size_t kMaxBlobSize = 64 * 1024;
    MLOGD("Sequencer", "LoadOnBoard sizes seq=%u meta=%u", (unsigned)seqSize, (unsigned)metaSize);
    if (seqSize == 0 || metaSize == 0) { MLOGE("Sequencer", "LoadOnBoard empty blobs seq=%u meta=%u", (unsigned)seqSize, (unsigned)metaSize); return false; }
    if (seqSize > kMaxBlobSize || metaSize > kMaxBlobSize) { MLOGE("Sequencer", "LoadOnBoard blob too large seq=%u meta=%u", (unsigned)seqSize, (unsigned)metaSize); return false; } // Prevent absurd allocations (e.g. storage not ready)
    std::vector<uint8_t> seqBuf(seqSize);
    std::vector<uint8_t> metaBuf(metaSize);
    MatrixOS::NVS::GetVariable(SEQUENCER_DATA_HASH, seqBuf.data(), seqBuf.size());
    MatrixOS::NVS::GetVariable(SEQUENCER_META_HASH, metaBuf.data(), metaBuf.size());
    SequenceData dataCopy = sequence.GetData();
    if (!DeserializeSequenceData(seqBuf.data(), seqBuf.size(), dataCopy)) { MLOGE("Sequencer", "DeserializeSequenceData failed"); return false; }
    SequenceMeta metaCopy = meta;
    if (!DeserializeSequenceMeta(metaBuf.data(), metaBuf.size(), metaCopy)) { MLOGE("Sequencer", "DeserializeSequenceMeta failed"); return false; }
    sequence.SetData(dataCopy);
    meta = metaCopy;
    MLOGD("Sequencer", "Sequence from On board slot %d loaded", slot);
    return true;
}

bool Sequencer::LoadSD(uint16_t slot)
{
    if (slot >= SD_SLOT_MAX) { MLOGD("Sequencer", "LoadSD slot out of range %u", slot); return false; }
    // TODO implmentation
    return false;
}

bool Sequencer::Saved(uint16_t slot)
{
    saveSlotType type = static_cast<saveSlotType>(slot >> 10);
    if (type == saveSlotType::OnBoard)
    {
        return SavedOnBoard(slot & 0x03FF);
    }
    else if (type == saveSlotType::SDCard)
    {
        return SavedSD(slot & 0x03FF);
    }
    return false;
}

bool Sequencer::SavedOnBoard(uint16_t slot)
{
    if (slot >= ONBOARD_SLOT_MAX) { MLOGD("Sequencer", "SavedOnBoard slot out of range %u", slot); return false; }
    size_t seqSize = MatrixOS::NVS::GetSize(SEQUENCER_DATA_HASH);
    size_t metaSize = MatrixOS::NVS::GetSize(SEQUENCER_META_HASH);
    return seqSize > 0 && metaSize > 0;
}

bool Sequencer::SavedSD(uint16_t slot)
{
    if (slot >= SD_SLOT_MAX) { MLOGD("Sequencer", "SavedSD slot out of range %u", slot); return false; }
    // TODO implementation
    return false;
}
