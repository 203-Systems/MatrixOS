#include "Sequencer.h"
#include "UI/UI.h"

#include "Scales.h"
#include "PatternSelector.h"
#include "TrackSelector.h"
#include "ClipLauncher.h"
#include "MixerControl.h"
#include "SequenceVisualizer.h"
#include "NotePad.h"
#include "ControlBar.h"

void Sequencer::Setup(const vector<string> &args)
{
    // Application initialization code here
    if (lastSequence.Get().empty())
    {
        sequence.New(8);
        meta.New(8);
    }
    else
    {
        // Load sequence
    }

    SequencerUI();
}

void Sequencer::SequencerUI()
{
    UI sequencerUI("SequencerUI", Color(0x00FFFF), false);

    uint8_t track = this->track;

    SequencePattern* pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, sequence.GetPosition(track).pattern);

    SequenceVisualizer sequenceVisualizer(this, &this->stepSelected, &this->noteSelected, &this->noteActive);
    sequenceVisualizer.OnSelect([&](uint8_t step) -> void
    {
        if(pattern == nullptr)
        {
            return;
        }

        if(ClearActive())
        {
            ClearStep(pattern, step);
        }
        else if(CopyActive() && stepSelected.size() >= 2) // Self is included
        {
            CopyStep(pattern, stepSelected[0], step);
        }
        else if(!noteSelected.empty())
        {
            for (const auto& [note, velocity] : noteSelected)
            {
                StepAddNote(pattern, step, note, velocity);
            }
        }
    });
    sequenceVisualizer.SetEnableFunc([&]() -> bool { return currentView == ViewMode::Sequencer; });
    sequencerUI.AddUIComponent(&sequenceVisualizer, Point(0, 1));

    SequencerNotePad notePad(this, &this->noteSelected, &this->noteActive);
    notePad.OnSelect([&](bool noteOn, uint8_t note, uint8_t velocity) -> void
    {
        if(pattern != nullptr && noteOn)
        {
            bool existAlready = false;

            for (const auto& step : stepSelected)
            {
                uint16_t startTime = step * Sequence::PPQN;
                uint16_t endTime = startTime + Sequence::PPQN - 1;

                if (pattern->RemoveNoteEventsInRange(startTime, endTime, note))
                {
                    existAlready = true;
                    noteActive.erase(noteActive.find(note));
                }
            }

            if(existAlready == false)
            {
                SequenceEvent event = SequenceEvent::Note(note, velocity, false);
                for (const auto& step : stepSelected)
                {
                    pattern->AddEvent(step * Sequence::PPQN, event);
                    noteActive.insert(note);
                }
            }
        }

        // TODO: Pass into Sequence to record

    });
    notePad.SetEnableFunc([&]() -> bool { return currentView == ViewMode::Sequencer; });
    sequencerUI.AddUIComponent(&notePad, Point(0, 3));

    PatternSelector patternSelector(this);
    patternSelector.OnChange([&](uint8_t patternIdx) -> void
    {
        ClearActiveNotes();
        pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, patternIdx);
    });
    patternSelector.SetEnableFunc([&]() -> bool { return currentView == ViewMode::Sequencer && (ShiftActive() || patternView); });
    sequencerUI.AddUIComponent(&patternSelector, Point(0, 3));

    // Session View
    ClipLauncher clipLauncher(this);
    clipLauncher.OnChange([&](uint8_t track, uint8_t clip) -> void
    {
        stepSelected.clear();

        ClearActiveNotes();
        ClearSelectedNotes();

        notePad.GenerateKeymap();

        pattern = &sequence.GetPattern(track, clip, sequence.GetPosition(track).pattern);
    });
    clipLauncher.SetEnableFunc([&]() -> bool { return currentView == ViewMode::Session; });
    sequencerUI.AddUIComponent(&clipLauncher, Point(0, 0));

    // Mixer View
    MixerControl mixerControl(this);
    mixerControl.SetEnableFunc([&]() -> bool { return currentView == ViewMode::Mixer; });
    sequencerUI.AddUIComponent(&mixerControl, Point(0, 0));

    // Global
    TrackSelector trackSelector(this);
    trackSelector.OnChange([&](uint8_t val) -> void
    {
        stepSelected.clear();

        ClearActiveNotes();
        ClearSelectedNotes();

        notePad.GenerateKeymap();

        track = this->track;

        pattern = &sequence.GetPattern(track, sequence.GetPosition(track).clip, sequence.GetPosition(track).pattern);
    });
    trackSelector.SetEnableFunc([&]() -> bool { return currentView != ViewMode::Session && currentView != ViewMode::Mixer; });
    sequencerUI.AddUIComponent(&trackSelector, Point(0, 0));

    ControlBar controlBar(this, &notePad);
    controlBar.OnClear([&]() -> void
    {
        for (const auto& step : stepSelected)
        {
            ClearStep(pattern, step);
        }
    });
    sequencerUI.AddUIComponent(&controlBar, Point(0, 7));

    sequencerUI.SetGlobalLoopFunc([&]() -> void
                                { sequence.Tick(); });

    sequencerUI.AllowExit(false);
    sequencerUI.SetKeyEventHandler([&](KeyEvent *keyEvent) -> bool
                                   {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == RELEASED)
      {
        // Clean up the state
        clear = false;
        copy = false;
        shift = 0;
        shiftEventOccured = false;

        ClearActiveNotes();
        ClearSelectedNotes();

        SequencerMenu();
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
    sequencerMenu.AddUIComponent(&trackSelector, Point(0, 0));

    // Right side, Track specific settings
    UIButton colorSelectorBtn;
    colorSelectorBtn.SetName("Color Selector");
    colorSelectorBtn.SetColorFunc([&]() -> Color
                                  {
        if(track < meta.tracks.size()) {
            return meta.tracks[track].color;
        }
        return Color::White; });
    colorSelectorBtn.OnPress([&]() -> void
                             {
        if(track < meta.tracks.size() && MatrixOS::UIUtility::ColorPicker(meta.tracks[track].color)) {
            sequence.SetDirty();
        } });
    sequencerMenu.AddUIComponent(&colorSelectorBtn, Point(7, 2));

    UIButton layoutSelectorBtn;
    layoutSelectorBtn.SetName("Layout Selector");
    layoutSelectorBtn.SetColor(Color(0xFFFF00));
    layoutSelectorBtn.OnPress([&]() -> void
                              { LayoutSelector(); });
    sequencerMenu.AddUIComponent(&layoutSelectorBtn, Point(7, 3));

    UIButton channelSelectorBtn;
    channelSelectorBtn.SetName("Channel Selector");
    channelSelectorBtn.SetColor(Color(0x60FF00));
    channelSelectorBtn.OnPress([&]() -> void
                               { ChannelSelector(); });
    sequencerMenu.AddUIComponent(&channelSelectorBtn, Point(7, 4));

    UIButton forceSensitiveToggle;
    forceSensitiveToggle.SetName("Velocity Sensitive");
    if(Device::KeyPad::velocity_sensitivity)
    {
        forceSensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(meta.tracks[track].velocitySensitive); });
        forceSensitiveToggle.OnPress([&]() -> void { meta.tracks[track].velocitySensitive = !meta.tracks[track].velocitySensitive; sequence.SetDirty(); });
        forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(forceSensitiveToggle.GetName() + " " + (meta.tracks[track].velocitySensitive ? "On" : "Off"), forceSensitiveToggle.GetColor()); });
    }
    else
    {
        forceSensitiveToggle.SetColor(Color(0x00FFB0).Dim());
        forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll("Velocity Sensitivity Not Supported", Color(0x00FFB0)); });
    }
    sequencerMenu.AddUIComponent(forceSensitiveToggle, Point(7, 5));

    // Left side, Sequencer Global settings
    UIButton bpmSelectorBtn;
    bpmSelectorBtn.SetName("BPM Selector");
    bpmSelectorBtn.SetColor(Color(0xFF0080));
    bpmSelectorBtn.OnPress([&]() -> void
                           { BPMSelector(); });
    sequencerMenu.AddUIComponent(&bpmSelectorBtn, Point(0, 2));

    UIButton swingSelectorBtn;
    swingSelectorBtn.SetName("Swing Selector");
    swingSelectorBtn.SetColor(Color(0xFFA000));
    swingSelectorBtn.OnPress([&]() -> void
                           { SwingSelector(); });
    sequencerMenu.AddUIComponent(&swingSelectorBtn, Point(0, 3));

    UIToggle clockOutputToggle;
    clockOutputToggle.SetName("Clock Output");
    clockOutputToggle.SetColor(Color(0x00A0FF));
    clockOutputToggle.SetValuePointer(&meta.clockOutput);
    clockOutputToggle.OnPress([&]() -> void { sequence.SetDirty(); });
    sequencerMenu.AddUIComponent(&clockOutputToggle, Point(0, 5));

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
    channelSelector.AddUIComponent(&trackSelector, Point(0, 0));

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
}

void Sequencer::SwingSelector()
{
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
    return shift > 0;
}

void Sequencer::ShiftEventOccured()
{
    shiftEventOccured = true;
}

void Sequencer::ClearActiveNotes()
{
    uint8_t channel = sequence.GetChannel(track);
    for (const auto& note : noteActive)
    {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
    }
    noteActive.clear();
}

void Sequencer::ClearSelectedNotes()
{
    uint8_t channel = sequence.GetChannel(track);
    for (const auto& [note, velocity] : noteSelected)
    {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
    }
    noteSelected.clear();
}

void Sequencer::ClearStep(SequencePattern* pattern, uint8_t step)
{
    uint16_t startTime = step * Sequence::PPQN;
    uint16_t endTime = startTime + Sequence::PPQN - 1;
    uint8_t channel = sequence.GetChannel(track);

    // Remove notes from noteActive and send noteOff
    auto it = pattern->events.lower_bound(startTime);
    while (it != pattern->events.end() && it->first <= endTime)
    {
        if (it->second.eventType == SequenceEventType::NoteEvent)
        {
            const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
            auto activeIt = noteActive.find(noteData.note);
            if(activeIt != noteActive.end())
            {
                MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0));
                noteActive.erase(activeIt);
            }
        }
        ++it;
    }

    pattern->RemoveAllEventsInRange(startTime, endTime);
}

void Sequencer::CopyStep(SequencePattern* pattern, uint8_t src, uint8_t dest)
{
    // Clear destination first
    ClearStep(pattern, dest);

    // Copy events
    uint16_t sourceStartTime = src * Sequence::PPQN;
    uint16_t destStartTime = dest * Sequence::PPQN;
    pattern->CopyEventsInRange(sourceStartTime, destStartTime, Sequence::PPQN);

    // Add copied notes to noteActive
    uint16_t destEndTime = destStartTime + Sequence::PPQN - 1;
    auto it = pattern->events.lower_bound(destStartTime);
    while (it != pattern->events.end() && it->first <= destEndTime)
    {
        if (it->second.eventType == SequenceEventType::NoteEvent)
        {
            const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
            noteActive.insert(noteData.note);
        }
        ++it;
    }
}

void Sequencer::StepAddNote(SequencePattern* pattern, uint8_t step, uint8_t note, uint8_t velocity, bool aftertouch)
{
    SequenceEvent event = SequenceEvent::Note(note, velocity, aftertouch);
    pattern->AddEvent(step * Sequence::PPQN, event);
    noteActive.insert(note);
}

void Sequencer::SetView(ViewMode view)
{
    if(currentView == view)
    {
        return;
    }

    if(currentView == ViewMode::Sequencer)
    {
        ClearSelectedNotes();
        ClearActiveNotes();
    }

    currentView = view;
}