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
#include "SaveButton.h"
#include "cb0r.h"

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

    sequence.EnableClockOutput(meta.clockOutput);

    if (tickTaskHandle == nullptr)
    {
        xTaskCreate(SequenceTask, "SeqTick", configMINIMAL_STACK_SIZE * 2, this, 1, &tickTaskHandle);
    }

    ClearState();
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

    PatternPad patternPad(this);
    sequencerUI.AddUIComponent(patternPad, Point(0, 1));

    SequencerNotePad notePad(this);
    sequencerUI.AddUIComponent(notePad, Point(0, 3));

    PatternSelector patternSelector(this);
    sequencerUI.AddUIComponent(patternSelector, Point(0, 3));

    // Session View
    ClipLauncher clipLauncher(this);
    sequencerUI.AddUIComponent(clipLauncher, Point(0, 0));

    // Mixer View
    MixerControl mixerControl(this);
    sequencerUI.AddUIComponent(mixerControl, Point(0, 0));

    // Step Detail View
    EventDetailView eventDetailView(this);
    sequencerUI.AddUIComponent(eventDetailView, Point(0, 0));

    // Global
    TrackSelector trackSelector(this);
    trackSelector.OnChange([&](uint8_t val) -> void
    {
        notePad.GenerateKeymap();
    });    
    trackSelector.SetEnableFunc([&]() -> bool
                                { return currentView != ViewMode::Session && currentView != ViewMode::Mixer && currentView != ViewMode::StepDetail; });
    sequencerUI.AddUIComponent(trackSelector, Point(0, 0));

    SequencerControlBar controlBar(this, &notePad);
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
            ClearState();

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

    SaveButton saveBtn(this);
    sequencerMenu.AddUIComponent(saveBtn, Point(2, 2));

    UIButton sequenceBrowserBtn;
    sequenceBrowserBtn.SetName("Sequence Browser");
    sequenceBrowserBtn.SetColorFunc([&]() -> Color {
        return MatrixOS::FileSystem::Available() ? ColorEffects::Rainbow(3000) : Color::Red.Dim();
    });
    sequenceBrowserBtn.SetSize(Dimension(4, 1));
    sequenceBrowserBtn.OnPress([&]() -> void
                               { 
                                MatrixOS::FileSystem::Available() ? 
                                SequenceBrowser() : 
                                MatrixOS::UIUtility::TextScroll("No SD Card", Color::Red);
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

void Sequencer::ClearState()
{
    clear = copy = false;
    shift[0] = shift[1] = false;
    shiftEventOccured[0] = shiftEventOccured[1] = false;
    ClearSelectedNotes();
    ClearActiveNotes();
    stepSelected.clear();
    copySourceStep = -1;
    trackSelected = false;
}

void Sequencer::LayoutSelector()
{
    Color noteColor = Color(0xFFFF00);
    Color drumColor = Color(0xFF8000);
    Color ccColor = Color(0x0080FF);
    Color pcColor = Color(0x00FF80);

    Color updateColor = Color(0x80FF00);
    
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

    SequencerScaleVisualizer scaleVisualizer(Color(0x8000FF), Color(0xFF0080),  Color(0xFF0080));
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

    // Update  text display
    UITimedDisplay updateTextDisplay(1000);
    updateTextDisplay.SetDimension(Dimension(8, 4));
    updateTextDisplay.SetRenderFunc([&](Point origin) -> void
                                       {
        // U
        MatrixOS::LED::SetColor(origin + Point(0, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(1, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 3), updateColor);

        // P
        MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);

        // D
        MatrixOS::LED::SetColor(origin + Point(5, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(6, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(6, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(7, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(7, 2), updateColor);
    });
    layoutSelector.AddUIComponent(updateTextDisplay, Point(0, 4));
    updateTextDisplay.Disable();

    // Apply to other note tracks
    UIButton applyNoteBtn;
    applyNoteBtn.SetName("Apply to other note track");
    applyNoteBtn.SetColorFunc([&]() -> Color {
        return updateTextDisplay.IsEnabled() ? Color::White : updateColor;
    });
    applyNoteBtn.SetEnableFunc([&]() -> bool {
        return meta.tracks[track].mode == SequenceTrackMode::NoteTrack;
    });
    applyNoteBtn.OnPress([&]() -> void {
        const auto& src = meta.tracks[track].config.note;
        for (size_t i = 0; i < meta.tracks.size(); ++i)
        {
            if (i == track) continue;
            if (meta.tracks[i].mode != SequenceTrackMode::NoteTrack) continue;
            auto& dst = meta.tracks[i].config.note;
            dst.type = src.type;
            dst.customScale = src.customScale;
            dst.enforceScale = src.enforceScale;
            dst.scale = src.scale;
            dst.root = src.root;
            dst.rootOffset = src.rootOffset;
            dst.octave = src.octave;
        }
        updateTextDisplay.Enable();
        sequence.SetDirty();
    });
    layoutSelector.AddUIComponent(applyNoteBtn, Point(7, 2));

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

    // Time text display
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

    // Update  text display
    UITimedDisplay updateTextDisplay(1000);
    updateTextDisplay.SetDimension(Dimension(8, 4));
    updateTextDisplay.SetRenderFunc([&](Point origin) -> void
                                       {
        // U
        MatrixOS::LED::SetColor(origin + Point(0, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(0, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(1, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(2, 3), updateColor);

        // P
        MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
        MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
        MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);

        // D
        MatrixOS::LED::SetColor(origin + Point(5, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 2), updateColor);
        MatrixOS::LED::SetColor(origin + Point(5, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(6, 0), updateColor);
        MatrixOS::LED::SetColor(origin + Point(6, 3), updateColor);
        MatrixOS::LED::SetColor(origin + Point(7, 1), updateColor);
        MatrixOS::LED::SetColor(origin + Point(7, 2), updateColor);
    });
    patternLengthSelector.AddUIComponent(updateTextDisplay, Point(0, 0));
    updateTextDisplay.Disable();

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
    {
        return !patternLengthTextDisplay.IsEnabled() && !updateTextDisplay.IsEnabled();
    });
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
                                updateTextDisplay.Disable();
                                updateTime = 0;
                            });

    patternLengthSelector.AddUIComponent(patternLengthInput, Point(0, 6));

    UIButton updatePatternsBtn;
    updatePatternsBtn.SetName("Update Empty Patterns");
    updatePatternsBtn.SetColorFunc([&]() -> Color
    {return updateTextDisplay.IsEnabled() ? Color::White : updateColor;});
    updatePatternsBtn.OnPress([&]() -> void {
        sequence.SetPatternLength(patternLength);
        sequence.UpdateEmptyPatternsWithPatternLength();
        patternLengthTextDisplay.Disable();
        updateTextDisplay.Enable();
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
        ClearState();
    }

    currentView = view;
}

bool Sequencer::Save(uint16_t slot)
{
    sequence.Stop();

    if (slot == 0xFFFF) { 
        MLOGD("Sequencer", "Load - No Previous Assigned Slot - Finding the next available slot"); 
        uint16_t freeSlot = 0xFFFF;
        for (uint16_t i = 0; i < SD_SLOT_MAX; ++i)
        {
            if (!Saved(i))
            {
                freeSlot = i;
                break;
            }
        }
        if (freeSlot == 0xFFFF)
        {
            MLOGE("Sequencer", "Save - no free slot available");
            return false;
        }
        slot = freeSlot;
        MLOGD("Sequencer", "Save - using free slot %u", slot);
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

    string slotDir = "/sequences/" + std::to_string(slot + 1);
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

    if (!BackupSlot(slot)) return false;

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
    sequence.SetDirty(false);
    return true;
}

bool Sequencer::Load(uint16_t slot)
{
    sequence.Stop();

    if (slot == 0xFFFF) { MLOGD("Sequencer", "Load - No Previous Assigned Slot"); return false; }
    if (slot >= SD_SLOT_MAX) { MLOGE("Sequencer", "Load - slot out of range %u", slot); return false; }
    if (!MatrixOS::FileSystem::Available()) { MLOGE("Sequencer", "Load - filesystem not available"); return false; }

    string slotDir = "/sequences/" + std::to_string(slot + 1);
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
    saveSlot = slot;
    sequence.SetDirty(false);
    MLOGD("Sequencer", "Loaded SD slot %u", slot);
    return true;
}

bool Sequencer::Saved(uint16_t slot)
{
    if (slot >= SD_SLOT_MAX) { MLOGD("Sequencer", "SavedSD slot out of range %u", slot); return false; }
    if (!MatrixOS::FileSystem::Available()) { return false; }
    string base = "/sequences/" + std::to_string(slot + 1) + "/";
    string dataPath = base + "sequence.data";
    string metaPath = base + "sequence.meta";
    return MatrixOS::FileSystem::Exists(dataPath) && MatrixOS::FileSystem::Exists(metaPath);
}

static void RenderUpArrow(Point origin, Color color)
{
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            if (x == 1 || x == 2 || y == 1)
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);    
            }
        }
    }
}

static void RenderDownArrow(Point origin, Color color)
{
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            if (x == 1 || x == 2 || y == 2)
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);    
            }
        }
    }
}

static void RenderRing(Point origin, Color color)
{
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            if (x == 0 || x == 3 || y == 0 || y == 3)
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }
    }
}

static void RenderCross(Point origin, Color color)
{
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            if (x == y || x == 3 - y) 
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }
    }
}

void Sequencer::ConfirmSaveUI()
{
    UI confirmSaveUI("Confirm Save", meta.color, false);
    bool saved = false;
    bool failed = false;
    uint32_t openTime = MatrixOS::SYS::Millis();
    uint32_t savedTime = 0;

    confirmSaveUI.SetPostRenderFunc([&]() -> void
                                    {
                                        if(saved == false)
                                        {
                                            uint8_t scale = ColorEffects::Strobe(500, openTime);
                                            RenderDownArrow(Point(2,2), scale ? Color::White : meta.color);
                                        }
                                        else if(saved == true)
                                        {
                                            RenderRing(Point(2,2), meta.color);
                                        }
                                        else if(failed == true)
                                        {
                                            RenderCross(Point(2,2), Color::Red);
                                        }   

                                        if(saved || failed)
                                        {
                                            if(MatrixOS::SYS::Millis() - savedTime > 500) {confirmSaveUI.Exit();}
                                        }
                                    });
    
    confirmSaveUI.SetKeyEventHandler([&](KeyEvent *keyEvent) -> bool
                                     {
            if((saved || failed))
            {
                if(keyEvent->info.state == PRESSED)
                {
                    confirmSaveUI.Exit();
                    KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(keyEvent->id);
                    if(keyInfo) {keyInfo->Clear();}
                }
                return true;
            }

            Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->id);
            if(xy && (xy.x == 3 || xy.x == 4 || xy.y == 5))
            {
                if (keyEvent->info.state == RELEASED && keyEvent->Hold() == false)
                {
                    RenderDownArrow(Point(2,2), Color::White);
                    bool success = Save(saveSlot);
                    savedTime = MatrixOS::SYS::Millis();
                    if(success)
                    {
                        saved = true;
                    }
                    else
                    {
                        failed = true;
                    }
                    return true;
                }
            }
            return false; });

    confirmSaveUI.Start();
}   

void Sequencer::SequenceBrowser()
{
    UI browser("Sequence Browser", meta.color, true);

    bool clear = false;
    bool copy = false;
    int8_t copySrc = -1;
    uint32_t modifierTime = 0;

    bool loaded = false;
    bool saved = false;
    bool failed = false;
    uint32_t eventTime = 0;

    // collect slot colors (8x6 grid), sparse map
    std::map<uint16_t, Color> slotColors;

    for (uint16_t slot = 0; slot < SD_SLOT_MAX && slot < 48; ++slot)
    {
        if (Saved(slot))
        {
            Color c = meta.color;
            std::string path = "/sequences/" + std::to_string(slot + 1) + "/sequence.meta";
            File f = MatrixOS::FileSystem::Open(path, "rb");
            if (GetColorFromSequenceMetaFile(f, c)) slotColors[slot] = c;
        }
    }

    browser.SetPostRenderFunc([&]() -> void
    {
        if(eventTime != 0 && MatrixOS::SYS::Millis() - eventTime < 500)
        {
            if(loaded)
            {
                RenderRing(Point(2,2), meta.color);
            }
            else if(saved)
            {
                RenderRing(Point(2,2), meta.color);
            }
            else if(failed)
            {
                RenderCross(Point(2,2), Color::Red);
            }
            return;
        }
        else if (eventTime != 0)
        {
            eventTime = 0;
            MatrixOS::LED::Fade();
        }

        if(modifierTime != 0 && MatrixOS::SYS::Millis() - modifierTime < 500)
        {
            if(clear)
            {
                Color color = Color(0xFF0080);

                // C
                MatrixOS::LED::SetColor(Point(0, 1), color);
                MatrixOS::LED::SetColor(Point(0, 2), color);
                MatrixOS::LED::SetColor(Point(0, 3), color);
                MatrixOS::LED::SetColor(Point(0, 4), color);
                MatrixOS::LED::SetColor(Point(1, 1), color);
                MatrixOS::LED::SetColor(Point(1, 4), color);
                MatrixOS::LED::SetColor(Point(2, 1), color);
                MatrixOS::LED::SetColor(Point(2, 4), color);
                
                // L
                MatrixOS::LED::SetColor(Point(3, 1), Color::White);
                MatrixOS::LED::SetColor(Point(3, 2), Color::White);
                MatrixOS::LED::SetColor(Point(3, 3), Color::White);
                MatrixOS::LED::SetColor(Point(3, 4), Color::White);
                MatrixOS::LED::SetColor(Point(4, 4), Color::White);

                // R
                MatrixOS::LED::SetColor(Point(5, 1), color);
                MatrixOS::LED::SetColor(Point(5, 2), color);
                MatrixOS::LED::SetColor(Point(5, 3), color);
                MatrixOS::LED::SetColor(Point(5, 4), color);
                MatrixOS::LED::SetColor(Point(6, 1), color);
                MatrixOS::LED::SetColor(Point(6, 3), color);
                MatrixOS::LED::SetColor(Point(7, 1), color);
                MatrixOS::LED::SetColor(Point(7, 2), color);
                MatrixOS::LED::SetColor(Point(7, 4), color);
            }
            else if(copy)
            {
                Color color = Color(0x8000FF);

                // C
                MatrixOS::LED::SetColor(Point(0, 1), color);
                MatrixOS::LED::SetColor(Point(0, 2), color);
                MatrixOS::LED::SetColor(Point(0, 3), color);
                MatrixOS::LED::SetColor(Point(0, 4), color);
                MatrixOS::LED::SetColor(Point(1, 1), color);
                MatrixOS::LED::SetColor(Point(1, 4), color);
                MatrixOS::LED::SetColor(Point(2, 1), color);
                MatrixOS::LED::SetColor(Point(2, 4), color);

                // P
                MatrixOS::LED::SetColor(Point(3, 1), Color::White);
                MatrixOS::LED::SetColor(Point(3, 2), Color::White);
                MatrixOS::LED::SetColor(Point(3, 3), Color::White);
                MatrixOS::LED::SetColor(Point(3, 4), Color::White);
                MatrixOS::LED::SetColor(Point(4, 1), Color::White);
                MatrixOS::LED::SetColor(Point(4, 2), Color::White);

                // Y
                MatrixOS::LED::SetColor(Point(5, 1), color);
                MatrixOS::LED::SetColor(Point(5, 2), color);
                MatrixOS::LED::SetColor(Point(6, 3), color);
                MatrixOS::LED::SetColor(Point(6, 4), color);
                MatrixOS::LED::SetColor(Point(7, 1), color);
                MatrixOS::LED::SetColor(Point(7, 2), color);
            }
        }
        else
        {
            for (uint8_t y = 0; y < 6; ++y)
            {
                for (uint8_t x = 0; x < 8; ++x)
                {
                    uint16_t idx = y * 8 + x;
                    Color color;
                    auto it = slotColors.find(idx);
                    bool slotPopulated = (it != slotColors.end());
                        
                    if(clear)
                    {
                        color = slotPopulated ? it->second : Color::Black;
                    }
                    else if (copy)
                    {
                        if(copySrc > 0)
                        {
                            if(idx == copySrc)
                            {
                                color = Color::White;
                            }
                            else
                            {
                                color = slotPopulated ? it->second : Color(0x101010);
                            }
                        }
                        else
                        {
                            color = slotPopulated ? it->second : Color::Black;
                        }
                    }
                    else
                    {
                        if(slotPopulated)
                        {
                            if (idx == saveSlot)
                            {
                                if(sequence.GetDirty())
                                {
                                    color = Color::Crossfade(it->second, Color::Red.Dim(), Fract16(ColorEffects::Breath(500), 8));
                                }
                                else
                                {
                                    color = Color::Crossfade(it->second, Color::White, Fract16(ColorEffects::Breath(500) / 4 * 3, 8));
                                }
                            }
                            else
                            {
                                color = it->second;
                            }
                        }
                        else
                        {
                            color = Color(0x101010);
                        }
                    }
                    MatrixOS::LED::SetColor(Point(x, y), color);
                }
            }
        }

        // Color picker strip (x 2..5, y 7)
        for (uint8_t x = 2; x < 6; ++x)
        {
            MatrixOS::LED::SetColor(Point(x, 7), meta.color);
        }

        // Clear button (0,7)
        MatrixOS::LED::SetColor(Point(0, 7), clear ? Color::White : Color(0xFF0080));

        // Copy button (7,7)
        MatrixOS::LED::SetColor(Point(7, 7), copy ? Color::White : Color(0x0080FF));
    });

    browser.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool
    {
        if(eventTime != 0 && MatrixOS::SYS::Millis() - eventTime < 500)
        {
            if(keyEvent->State() == PRESSED)
            {
                eventTime = 0;
                MatrixOS::LED::Fade();
                KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(keyEvent->id);
                if(keyInfo) {keyInfo->Clear();}
            }
            return true;
        }

        Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->id);
        if(!xy) {return false;}
        if (xy.y >= 0 && xy.y < 6 && xy.x >= 0 && xy.x < 8)
        {
            if(modifierTime != 0 && MatrixOS::SYS::Millis() - modifierTime < 500)
            {
                return true;
            }

            if (keyEvent->info.state == HOLD)
            {
                uint16_t slot = xy.y * 8 + xy.x;
                auto it = slotColors.find(slot);
                bool hasSequence = (it != slotColors.end());
                if (hasSequence)
                {
                    MatrixOS::UIUtility::TextScroll("Sequence #" + std::to_string(slot + 1), it->second);
                }
                else
                {
                    MatrixOS::UIUtility::TextScroll("Empty Slot", Color::White);
                }
                clear = false;
                copy = false;
            }
            else if (keyEvent->info.state == RELEASED && keyEvent->info.Hold() == false)
            {
                uint16_t slot = xy.y * 8 + xy.x;
                auto it = slotColors.find(slot);
                bool hasSequence = (it != slotColors.end());
                if(clear)
                {
                    if (hasSequence)
                    {
                        if(ClearSlot(slot))
                        {
                            slotColors.erase(slot);
                        }
                        else
                        {
                            failed = true;
                            eventTime = MatrixOS::SYS::Millis();
                            clear = false;
                            modifierTime = 0;
                        }
                    }
                }
                else if(copy)
                {
                    if(copySrc < 0)
                    {
                        if (hasSequence)
                        {
                            copySrc = slot;
                        }
                    }
                    else
                    {
                        if(CopySlot(copySrc, slot))
                        {
                            slotColors[slot] = slotColors[copySrc];
                        }
                        else
                        {
                            failed = true;
                            eventTime = MatrixOS::SYS::Millis();
                        }
                        copy = false;
                        copySrc = -1;
                    }
                }
                else
                {
                    if (hasSequence && slot != saveSlot)
                    {
                        MatrixOS::LED::Fill(Color::Black);
                        RenderUpArrow(Point(2, 2), Color::White);
                        MatrixOS::LED::Update();
                        loaded = false;
                        saved = false;
                        failed = false;
                        if (Load(slot)) {
                            loaded = true;
                        }
                        else { 
                            failed = true;
                        }
                        eventTime = MatrixOS::SYS::Millis();
                    }
                    else
                    {
                        MatrixOS::LED::Fill(Color::Black);
                        RenderDownArrow(Point(2, 2), Color::White);
                        MatrixOS::LED::Update();
                        loaded = false;
                        saved = false;
                        failed = false;
                        if (Save(slot))
                        {
                            slotColors[slot] = meta.color;
                            saved = true;
                        }
                        else
                        {   
                            failed = true;
                        }
                        eventTime = MatrixOS::SYS::Millis();
                    }
            }
            }
            return true;
        }
        else if (xy.x > 1 && xy.x < 6 && xy.y == 7) // Color Picker
        {
            if (keyEvent->info.state == HOLD)
            {
                MatrixOS::UIUtility::TextScroll("Sequence Color", meta.color);
                clear = false;
                copy = false;
            }
            else if (keyEvent->info.state == RELEASED && keyEvent->info.Hold() == false)
            {
                Color newColor = meta.color;
                if (MatrixOS::UIUtility::ColorPicker(newColor, false))
                {
                    meta.color = newColor;
                    sequence.SetDirty();
                }
                return true;
            }
            return true;
        }
        else if (xy.x == 0 && xy.y == 7) // Clear
        {
            if (keyEvent->info.state == PRESSED)
            {
                clear = true;
                copy = false;
                modifierTime = MatrixOS::SYS::Millis();
            }
            else if (keyEvent->info.state == RELEASED)
            {
                if(clear)
                {
                    clear = false;
                    modifierTime = 0;
                }
            }
            return true;
        }
        else if (xy.x == 7 && xy.y == 7) // Copy
        {
            if (keyEvent->info.state == PRESSED)
            {
                copy = true;
                clear = false;
                copySrc = -1;
                modifierTime = MatrixOS::SYS::Millis();
            }
            else if (keyEvent->info.state == RELEASED)
            {
                if(copy)
                {
                    copy = false;
                    copySrc = -1;
                    modifierTime = 0;
                }
            }
            return true;
        }
        else
        {
            if(keyEvent->info.state == HOLD)
            {
                copy = false;
                clear = false;
                MatrixOS::UIUtility::TextScroll(browser.name, Color::White);
            }
        }

        return false;
    });

    browser.Start();
}

bool Sequencer::ClearSlot(uint16_t slot)
{
    if (!MatrixOS::FileSystem::Available()) return false;
    std::string base = "/sequences/" + std::to_string(slot + 1) + "/";
    if(slot == saveSlot) {
        saveSlot = 0xFFFF;
        sequence.SetDirty();
    }
    bool ok1 = MatrixOS::FileSystem::Remove(base + "sequence.data");
    bool ok2 = MatrixOS::FileSystem::Remove(base + "sequence.meta");
    return ok1 || ok2;
}

bool Sequencer::CopySlot(uint16_t from, uint16_t to)
{
    if (from == to) return false;
    if (!MatrixOS::FileSystem::Available()) return false;
    if (!Saved(from)) return false;

    std::string fromBase = "/sequences/" + std::to_string(from + 1) + "/";
    std::string toBase   = "/sequences/" + std::to_string(to + 1) + "/";

    std::string fromData = fromBase + "sequence.data";
    std::string fromMeta = fromBase + "sequence.meta";

    std::string toData   = toBase + "sequence.data";
    std::string toMeta   = toBase + "sequence.meta";

    MatrixOS::FileSystem::MakeDir("/sequences");
    MatrixOS::FileSystem::MakeDir(toBase);

    // backup existing dest
    if (!BackupSlot(to))
    {
        MLOGE("Sequencer", "CopySlot - backup dest slot %u failed", to);
        return false;
    }

    if(to == saveSlot)
    {
        saveSlot = 0xFFFF;
        sequence.SetDirty();
    }

    auto copyFile = [](const std::string& src, const std::string& dst) -> bool
    {
        File in = MatrixOS::FileSystem::Open(src, "rb");
        if (in.Name().empty()) return false;
        File out = MatrixOS::FileSystem::Open(dst, "wb");
        if (out.Name().empty()) return false;
        uint8_t buf[512];
        size_t n;
        while ((n = in.Read(buf, sizeof(buf))) > 0)
        {
            if (out.Write(buf, n) != n) return false;
        }
        in.Close();
        out.Close();
        return true;
    };

    bool ok1 = copyFile(fromData, toData);
    bool ok2 = copyFile(fromMeta, toMeta);

    // TOOD: If failed, restore from the back up
    
    return ok1 && ok2;
}

bool Sequencer::BackupSlot(uint16_t slot)
{
    std::string slotDir = "/sequences/" + std::to_string(slot + 1);
    std::string dataPath = slotDir + "/sequence.data";
    std::string metaPath = slotDir + "/sequence.meta";
    std::string prevDir = slotDir + "/prev";
    std::string prevData = prevDir + "/sequence.data";
    std::string prevMeta = prevDir + "/sequence.meta";

    if (!MatrixOS::FileSystem::Exists(prevDir))
    {
        if (!MatrixOS::FileSystem::MakeDir(prevDir))
        {
            MLOGE("Sequencer", "BackupSlot - failed to create %s", prevDir.c_str());
            return false;
        }
        MLOGD("Sequencer", "BackupSlot - created %s", prevDir.c_str());
    }

    if (MatrixOS::FileSystem::Exists(dataPath))
    {
        MatrixOS::FileSystem::Remove(prevData);
        if (!MatrixOS::FileSystem::Rename(dataPath, prevData))
        {
            MLOGE("Sequencer", "BackupSlot - failed to backup data to %s", prevData.c_str());
            return false;
        }
        MLOGD("Sequencer", "BackupSlot - backed up data to %s", prevData.c_str());
    }

    if (MatrixOS::FileSystem::Exists(metaPath))
    {
        MatrixOS::FileSystem::Remove(prevMeta);
        if (!MatrixOS::FileSystem::Rename(metaPath, prevMeta))
        {
            MLOGE("Sequencer", "BackupSlot - failed to backup meta to %s", prevMeta.c_str());
            return false;
        }
        MLOGD("Sequencer", "BackupSlot - backed up meta to %s", prevMeta.c_str());
    }

    return true;
}
