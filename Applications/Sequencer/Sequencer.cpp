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

void Sequencer::SequenceTask(void* ctx)
{
    Sequencer* self = static_cast<Sequencer*>(ctx);
    for (;;)
    {
        self->sequence.Tick();
        taskYIELD(); // yield immediately, keep highest-rate scheduling
    }
}

void Sequencer::Setup(const vector<string> &args)
{   
    if (!Load(saveSlot))
    {
        sequence.New(8);
        meta.New(8);
        saveSlot = 0xFFFF;
    }

    sequence.EnableClockOutput(meta.clockOutput);\
    sequence.SetDirty(false);

    if (tickTaskHandle == nullptr)
    {
        xTaskCreate(SequenceTask, "SeqTick", configMINIMAL_STACK_SIZE * 2, this, 1, &tickTaskHandle);
    }

    SequencerUI();
}

void Sequencer::End()
{
    sequence.Stop();
    if (tickTaskHandle)
    {
        vTaskDelete(tickTaskHandle);
        tickTaskHandle = nullptr;
    }
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
    UIButton bpmSelectorBtn;
    bpmSelectorBtn.SetName("BPM Selector");
    bpmSelectorBtn.SetColor(Color(0xFF0080));
    bpmSelectorBtn.OnPress([&]() -> void
                           { BPMSelector(); });
    sequencerMenu.AddUIComponent(bpmSelectorBtn, Point(0, 2));

    UIButton swingSelectorBtn;
    swingSelectorBtn.SetName("Swing Selector");
    swingSelectorBtn.SetColor(Color(0xFFA000));
    swingSelectorBtn.OnPress([&]() -> void
                             { SwingSelector(); });
    sequencerMenu.AddUIComponent(swingSelectorBtn, Point(0, 3));

    UIButton timeSignatureSelector;
    timeSignatureSelector.SetName("Time Signature Selector");
    timeSignatureSelector.SetColor(Color(0x8000FF));
    timeSignatureSelector.OnPress([&]() -> void
                                 { TimeSignatureSelector(); });
    sequencerMenu.AddUIComponent(timeSignatureSelector, Point(0, 4));

    UIButton patternLengthSelectorBtn;
    patternLengthSelectorBtn.SetName("Pattern Length Selector");
    patternLengthSelectorBtn.SetColor(Color(0x00A0FF));
    patternLengthSelectorBtn.OnPress([&]() -> void
                                 { PatternLengthSelector(); });
    sequencerMenu.AddUIComponent(patternLengthSelectorBtn, Point(0, 5));

    UIButton playBtn;
    playBtn.SetName("Play");
    playBtn.SetColorFunc([&]() -> Color
                         {
                            if(sequence.Playing())
                            {
                                uint8_t scale = sequence.QuarterNoteProgressBreath() / 4 * 3;
                                return Color::Crossfade(Color::Green, Color::White, Fract16(scale, 8));
                            }
                            return Color::Green;
                         });
    playBtn.OnPress([&]() -> void { sequence.Playing() ? sequence.Stop() : sequence.Play(); });
    sequencerMenu.AddUIComponent(playBtn, Point(0, 7));

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

void Sequencer::TimeSignatureSelector()
{
    Color color = Color(0x8000FF);
    Color color2 = Color(0xFF0080);
    UI timeSignatureSelector("Time Signature Selector", color, false);

    int32_t beatsPerBar = sequence.GetBeatsPerBar();
    int32_t beatUnit = sequence.GetBeatUnit();
    bool modified = false;

    // Pattern Length text display
    UITimedDisplay textDisplay(500);
    textDisplay.SetDimension(Dimension(8, 4));
    textDisplay.SetRenderFunc([&](Point origin) -> void
                                       {
        // T
        MatrixOS::LED::SetColor(origin + Point(0, 0), color);
        MatrixOS::LED::SetColor(origin + Point(1, 0), color);
        MatrixOS::LED::SetColor(origin + Point(1, 1), color);
        MatrixOS::LED::SetColor(origin + Point(1, 2), color);
        MatrixOS::LED::SetColor(origin + Point(1, 3), color);
        MatrixOS::LED::SetColor(origin + Point(2, 0), color);

        // I
        MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);

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
        MatrixOS::LED::SetColor(origin + Point(7, 3), color);
    });
    timeSignatureSelector.AddUIComponent(textDisplay, Point(0, 0));

    UI4pxNumber beatUnitDisplay;
    beatUnitDisplay.SetColor(color2);
    beatUnitDisplay.SetAlternativeColor(Color::White);
    beatUnitDisplay.SetDigits(2);
    beatUnitDisplay.SetValuePointer(&beatUnit);
    beatUnitDisplay.SetSpacing(0);
    beatUnitDisplay.SetEnableFunc([&]() -> bool { return !textDisplay.IsEnabled(); });
    timeSignatureSelector.AddUIComponent(beatUnitDisplay, Point(2, 0));
    
    UI4pxNumber beatsDisplay;
    beatsDisplay.SetColor(color);
    beatsDisplay.SetAlternativeColor(Color::White);
    beatsDisplay.SetDigits(2);
    beatsDisplay.SetValuePointer(&beatsPerBar);
    beatsDisplay.SetSpacing(0);
    beatsDisplay.SetEnableFunc([&]() -> bool { return !textDisplay.IsEnabled(); });
    timeSignatureSelector.AddUIComponent(beatsDisplay, Point(-2, 0));

    UIButton beatsDecreaseBtn;
    beatsDecreaseBtn.SetName("Beats -1");
    beatsDecreaseBtn.SetSize(Dimension(1, 1));
    beatsDecreaseBtn.SetColorFunc([&]() -> Color {
        return Color(0xFF00FF).DimIfNot(beatsPerBar > 1);
    });
    beatsDecreaseBtn.OnPress([&]() -> void {
        if (beatsPerBar > 1) {
            beatsPerBar--;
            modified = true;
            textDisplay.Disable();
        }
    });
    timeSignatureSelector.AddUIComponent(beatsDecreaseBtn, Point(1, 7));

    UIButton beatsIncreaseBtn;
    beatsIncreaseBtn.SetName("Beats +1");
    beatsIncreaseBtn.SetSize(Dimension(1, 1));
    beatsIncreaseBtn.SetColorFunc([&]() -> Color {
        return Color(0x00FFFF).DimIfNot(beatsPerBar < 16);
    });
    beatsIncreaseBtn.OnPress([&]() -> void {
        if (beatsPerBar < 16) {
            beatsPerBar++;
            modified = true;
            textDisplay.Disable();
        }
    });
    timeSignatureSelector.AddUIComponent(beatsIncreaseBtn, Point(2, 7));

    UIButton beatUnitDecreaseBtn;
    beatUnitDecreaseBtn.SetName("Beat Unit -1");
    beatUnitDecreaseBtn.SetSize(Dimension(1, 1));
    beatUnitDecreaseBtn.SetColorFunc([&]() -> Color {
        return Color(0xFF00FF).DimIfNot(beatUnit > 1);
    });
    beatUnitDecreaseBtn.OnPress([&]() -> void {
        if (beatUnit == 2) { beatUnit = 1; }
        else if (beatUnit == 4) { beatUnit = 2; }
        else if (beatUnit == 8) { beatUnit = 4; }
        else if (beatUnit == 16) { beatUnit = 8; }
        else { return; }
        modified = true;
        textDisplay.Disable();
    });
    timeSignatureSelector.AddUIComponent(beatUnitDecreaseBtn, Point(5, 7));

    UIButton beatUnitIncreaseBtn;
    beatUnitIncreaseBtn.SetName("Beat Unit +1");
    beatUnitIncreaseBtn.SetSize(Dimension(1, 1));
    beatUnitIncreaseBtn.SetColorFunc([&]() -> Color {
        return Color(0x00FFFF).DimIfNot(beatUnit < 16);
    });
    beatUnitIncreaseBtn.OnPress([&]() -> void {
        if (beatUnit == 1) { beatUnit = 2; }
        else if (beatUnit == 2) { beatUnit = 4; }
        else if (beatUnit == 4) { beatUnit = 8; }
        else if (beatUnit == 8) { beatUnit = 16; }
        else { return; }
        modified = true;
        textDisplay.Disable();
    });
    timeSignatureSelector.AddUIComponent(beatUnitIncreaseBtn, Point(6, 7));

    timeSignatureSelector.Start();

    if (modified)
    {
        sequence.SetTimeSignature(beatsPerBar, beatUnit);
    }
}

void Sequencer::PatternLengthSelector()
{
    Color color = Color(0x00A0FF);
    Color updateColor = Color(0x80FF00);
    uint32_t updateTime = 0;
    UI patternLengthSelector("Pattern Length Selector", color, false);
    int32_t patternLength = sequence.GetPatternLength();
    int32_t offsettedPatternLength = patternLength - 1;

    // Pattern Length text display
    UITimedDisplay patternLengthTextDisplay(500);
    patternLengthTextDisplay.SetDimension(Dimension(8, 4));
    patternLengthTextDisplay.SetRenderFunc([&](Point origin) -> void
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
    patternLengthSelector.AddUIComponent(patternLengthTextDisplay, Point(0, 0));

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
    numDisplay.SetValuePointer(&patternLength);
    numDisplay.SetSpacing(1);
    numDisplay.SetEnableFunc([&]() -> bool
                             { return !patternLengthTextDisplay.IsEnabled(); });
    patternLengthSelector.AddUIComponent(numDisplay, Point(1, 0));

    UISelector patternLengthInput;
    patternLengthInput.SetDimension(Dimension(8, 2));
    patternLengthInput.SetName("Pattern Length");
    patternLengthInput.SetColorFunc([&]() -> Color
                           {return (MatrixOS::SYS::Millis() - updateTime < 1000) ?  updateColor : color;});
    patternLengthInput.SetCount(64);
    patternLengthInput.SetValuePointer((uint16_t *)&offsettedPatternLength);
    patternLengthInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
    patternLengthInput.OnChange([&](uint16_t val) -> void
                            {
                                patternLength = val + 1;
                                patternLengthTextDisplay.Disable();
                                updateTime = 0;
                            });

    patternLengthSelector.AddUIComponent(patternLengthInput, Point(0, 6));

    UIButton updatePatternsBtn;
    updatePatternsBtn.SetName("Update Empty Patterns");
    updatePatternsBtn.SetColor(updateColor);
    updatePatternsBtn.OnPress([&]() -> void {
        sequence.SetPatternLength(patternLength);
        sequence.UpdateEmptyPatternsWithPatternLength();
        updateTime = MatrixOS::SYS::Millis();
    });
    patternLengthSelector.AddUIComponent(updatePatternsBtn, Point(7, 5));

    patternLengthSelector.Start();

    if (sequence.GetPatternLength() != patternLength)
    {
        sequence.SetPatternLength(patternLength);
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
    sequence.Stop();

    if (slot == 0xFFFF) { 
        MLOGD("Sequencer", "Load - No Previous Assigned Slot"); 
        // TODO open picker to save to
        slot = 0; // Set to 0 for now
    } 

    if (slot >= SD_SLOT_MAX) { MLOGE("Sequencer", "Save - slot out of range %u", slot); return false; }
    if (!MatrixOS::FileSystem::Available()) { MLOGE("Sequencer", "Save - filesystem not available"); return false; }

    if (!MatrixOS::FileSystem::Exists("/sequences") && !MatrixOS::FileSystem::MakeDir("/sequences"))
    {
        MLOGE("Sequencer", "Save - failed to create /sequences");
        return false;
    }
    else if (!MatrixOS::FileSystem::Exists("/sequences"))
    {
        MLOGD("Sequencer", "Save - created /sequences");
    }

    string slotDir = "/sequences/" + std::to_string(slot);
    if (!MatrixOS::FileSystem::Exists(slotDir))
    {
        if (!MatrixOS::FileSystem::MakeDir(slotDir))
        {
            MLOGE("Sequencer", "Save - failed to create %s", slotDir.c_str());
            return false;
        }
        MLOGD("Sequencer", "Save - created slot dir %s", slotDir.c_str());
    }

    // Sequence file paths
    string dataPath = slotDir + "/sequence.data";
    string metaPath = slotDir + "/sequence.meta";

    // Backup file paths
    string prevDir = slotDir + "/prev";
    string prevData = prevDir + "/sequence.data";
    string prevMeta = prevDir + "/sequence.meta";

    if (!MatrixOS::FileSystem::Exists(prevDir))
    {
        if (!MatrixOS::FileSystem::MakeDir(prevDir))
        {
            MLOGE("Sequencer", "Save - failed to create %s", prevDir.c_str());
            return false;
        }
        MLOGD("Sequencer", "Save - created backup dir %s", prevDir.c_str());
    }

    // Backup existing files if present
    if (MatrixOS::FileSystem::Exists(dataPath))
    {
        MatrixOS::FileSystem::Remove(prevData);
        if (!MatrixOS::FileSystem::Rename(dataPath, prevData))
        {
            MLOGE("Sequencer", "Save - failed to backup data to %s", prevData.c_str());
        }
        else
        {
            MLOGD("Sequencer", "Save - backed up data to %s", prevData.c_str());
        }
    }

    if (MatrixOS::FileSystem::Exists(metaPath))
    {
        MatrixOS::FileSystem::Remove(prevMeta);
        if (!MatrixOS::FileSystem::Rename(metaPath, prevMeta))
        {
            MLOGE("Sequencer", "Save - failed to backup meta to %s", prevMeta.c_str());
        }
        else
        {
            MLOGD("Sequencer", "Save - backed up meta to %s", prevMeta.c_str());
        }
    }

    // Write meta first
    File metaFile = MatrixOS::FileSystem::Open(metaPath, "wb");
    if (metaFile.Name().empty()) { MLOGE("Sequencer", "Save - open meta fail %s", metaPath.c_str()); return false; }
    bool metaOk = SerializeSequenceMeta(meta, metaFile);
    size_t metaFileSize = metaFile.Size();
    metaFile.Close();
    if (!metaOk) { MLOGE("Sequencer", "Save - write meta failed"); return false; }
    MLOGD("Sequencer", "Save - sequence meta written to %s, size=%u", metaPath.c_str(), (unsigned)metaFileSize);

    // Write sequence data
    File dataFile = MatrixOS::FileSystem::Open(dataPath, "wb");
    if (dataFile.Name().empty()) { MLOGE("Sequencer", "Save - open fail %s", dataPath.c_str()); return false; }
    bool dataOk = SerializeSequenceData(sequence.GetData(), dataFile);
    size_t dataFileSize = dataFile.Size();
    dataFile.Close();
    if (!dataOk) { MLOGE("Sequencer", "Save - serialize stream failed"); return false; }
    MLOGD("Sequencer", "Save - sequence data written to %s size=%u", dataPath.c_str(), (unsigned)dataFileSize);

    saveSlot = slot;
    return true;
}

bool Sequencer::Load(uint16_t slot)
{
    sequence.Stop();

    if (slot == 0xFFFF) { MLOGD("Sequencer", "Load - No Previous Assigned Slot"); return false; }
    if (slot >= SD_SLOT_MAX) { MLOGE("Sequencer", "Load - slot out of range %u", slot); return false; }
    if (!MatrixOS::FileSystem::Available()) { MLOGE("Sequencer", "Load - filesystem not available"); return false; }

    string slotDir = "/sequences/" + std::to_string(slot);
    string dataPath = slotDir + "/sequence.data";
    string metaPath = slotDir + "/sequence.meta";

    // Ensure slot and prev directories exist
    if (!MatrixOS::FileSystem::Exists("/sequences"))
    {
        MLOGE("Sequencer", "Save - /sequences does not exist");
        return false;
    }
    if (!MatrixOS::FileSystem::Exists(slotDir))
    {
        MLOGE("Sequencer", "Save - slot dir %s does not exist", slotDir.c_str());
        return false;
    }

    // Read meta first
    File metaFile = MatrixOS::FileSystem::Open(metaPath, "rb");
    if (metaFile.Name().empty()) { MLOGE("Sequencer", "Load - open meta fail %s", metaPath.c_str()); return false; }
    SequenceMeta metaCopy = meta;
    bool metaOk = DeserializeSequenceMeta(metaFile, metaCopy);
    metaFile.Close();
    if (!metaOk) { MLOGE("Sequencer", "Load - deserialize meta failed"); return false; }
    MLOGD("Sequencer", "Loaded sequence meta from %s", metaPath.c_str());

    // Read sequence data
    File dataFile = MatrixOS::FileSystem::Open(dataPath, "rb");
    if (dataFile.Name().empty()) { MLOGE("Sequencer", "Load - open fail %s", dataPath.c_str()); return false; }
    SequenceData dataCopy = sequence.GetData();
    bool dataOk = DeserializeSequenceData(dataFile, dataCopy);
    dataFile.Close();
    if (!dataOk) { MLOGE("Sequencer", "Load - deserialize data failed"); return false; }
    MLOGD("Sequencer", "Loaded sequence data from %s", dataPath.c_str());


    sequence.SetData(dataCopy);
    meta = metaCopy;
    MLOGD("Sequencer", "Loaded SD slot %u", slot);
    return true;
}

bool Sequencer::Saved(uint16_t slot)
{
    if (slot >= SD_SLOT_MAX) { MLOGD("Sequencer", "SavedSD slot out of range %u", slot); return false; }
    if (!MatrixOS::FileSystem::Available()) { return false; }
    string base = "/sequences/" + std::to_string(slot) + "/";
    string dataPath = base + "sequence.data";
    string metaPath = base + "sequence.meta";
    return MatrixOS::FileSystem::Exists(dataPath) && MatrixOS::FileSystem::Exists(metaPath);
}
