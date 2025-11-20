#include "Sequencer.h"
#include "UI/UI.h"

#include "Scales.h"
#include "TrackSelector.h"
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

    trackPatternIdx.resize(sequence.GetTrackCount());
    for (uint8_t i = 0; i < sequence.GetTrackCount(); i++)
    {
        trackPatternIdx[i] = 0;
    }

    SequencerUI();
}

void Sequencer::SequencerUI()
{
    UI sequencerUI("SequencerUI", Color(0x00FFFF), false);

    uint8_t track = this->track;

    SequencePattern& pattern = sequence.GetPattern(track, trackPatternIdx[track]);

    vector<uint8_t> stepSelected;
    std::unordered_map<uint8_t, uint8_t> noteSelected;
    std::unordered_set<uint8_t> noteActive;

    SequenceVisualizer sequenceVisualizer(this, &stepSelected, &noteActive);
    sequencerUI.AddUIComponent(&sequenceVisualizer, Point(0, 1));

    SequencerNotePad notePad(this, &noteSelected, &noteActive);
    notePad.OnSelect([&](bool noteOn, uint8_t note, uint8_t velocity) -> void
    {
        if(noteOn)
        {
            bool existAlready = false;

            for (const auto& step : stepSelected)
            {
                uint16_t startTime = step * Sequence::PPQN;
                uint16_t endTime = startTime + Sequence::PPQN - 1;

                if (pattern.RemoveNoteEventsInRange(startTime, endTime, note))
                {
                    existAlready = true;
                }
            }

            if(existAlready == false)
            {
                SequenceEvent event = SequenceEvent::Note(note, velocity, false);
                for (const auto& step : stepSelected)
                {
                    pattern.AddEvent(step * Sequence::PPQN, event);
                    noteActive.insert(note);
                }
            }
        }

        // TODO: Pass into Sequence for record

    });
    sequencerUI.AddUIComponent(&notePad, Point(0, 3));

    ControlBar controlBar(this);
    sequencerUI.AddUIComponent(&controlBar, Point(0, 7));

    TrackSelector trackSelector(this);
    trackSelector.OnChange([&](uint8_t val) -> void
    {
        stepSelected.clear();
        noteActive.clear();

        uint8_t channel = sequence.GetChannel(track);

        for (const auto& [note, velocity] : noteSelected)
        {
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
        }
        noteSelected.clear();

        notePad.GenerateKeymap();

        track = this->track;
    });
    
    sequencerUI.AddUIComponent(&trackSelector, Point(0, 0));

    sequencerUI.SetGlobalLoopFunc([&]() -> void
                                  { sequence.Tick(); });

    sequencerUI.AllowExit(false);
    sequencerUI.SetKeyEventHandler([&](KeyEvent *keyEvent) -> bool
                                   {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == RELEASED)
      {
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

    TrackSelector trackSelector(this);
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
