#include "Sequencer.h"
#include "UI/UI.h"

#include "Scales.h"
#include "TrackSelector.h"
#include "SequenceVisualizer.h"
#include "ControlBar.h"


void Sequencer::Setup(const vector<string>& args)
{
    // Application initialization code here
    if(lastSequence.Get().empty())
    {
        sequence.New(8);
        meta.New(8);
    }
    else
    {
        // Load sequence
    }

    trackPatternIdx.resize(sequence.GetTrackCount());
    for(uint8_t i = 0; i < sequence.GetTrackCount(); i++)
    {
        trackPatternIdx[i] = -1;
    }

    SequencerUI();
}

void Sequencer::SequencerUI()
{
    UI sequencerUI("SequencerUI", Color(0x00FFFF), false);
    
    vector<uint8_t> stepSelected;
    vector<uint8_t> noteSelected;

    sequencerUI.SetPreRenderFunc([&]() -> void {
        if(trackPatternIdx[track] < 0)
        {
            SequencePosition& position = sequence.GetPosition(track);
            trackPatternIdx[track] = position.pattern;
        }
    });

    TrackSelector trackSelector(this);
    sequencerUI.AddUIComponent(&trackSelector, Point(0, 0));

    SequenceVisualizer sequenceVisualizer(this, &stepSelected);
    sequencerUI.AddUIComponent(&sequenceVisualizer, Point(0, 1));

    ControlBar controlBar(this);
    sequencerUI.AddUIComponent(&controlBar, Point(0, 7));

    sequencerUI.SetGlobalLoopFunc([&]() -> void {
        sequence.Tick();
    });

    sequencerUI.AllowExit(false);
    sequencerUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == RELEASED)
      {
        SequencerMenu();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

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
    colorSelectorBtn.SetColorFunc([&]() -> Color {
        if(track < meta.tracks.size()) {
            return meta.tracks[track].color;
        }
        return Color::White;
    });
    colorSelectorBtn.OnPress([&]() -> void {
        if(track < meta.tracks.size() && MatrixOS::UIUtility::ColorPicker(meta.tracks[track].color)) {
            sequence.SetDirty();
        }
    });
    sequencerMenu.AddUIComponent(&colorSelectorBtn, Point(7, 2));

    UIButton layoutSelectorBtn;
    layoutSelectorBtn.SetName("Layout Selector");
    layoutSelectorBtn.SetColor(Color(0xFFFF00));
    layoutSelectorBtn.OnPress([&]() -> void { LayoutSelector(); });
    sequencerMenu.AddUIComponent(&layoutSelectorBtn, Point(7, 3));

    UIButton channelSelectorBtn;
    channelSelectorBtn.SetName("Channel Selector");
    channelSelectorBtn.SetColor(Color(0x60FF00));
    channelSelectorBtn.OnPress([&]() -> void { ChannelSelector(); });
    sequencerMenu.AddUIComponent(&channelSelectorBtn, Point(7, 4));

    // Left side, Sequencer Global settings
    UIButton bpmSelectorBtn;
    bpmSelectorBtn.SetName("BPM Selector");
    bpmSelectorBtn.SetColor(Color(0xFF0080));
    bpmSelectorBtn.OnPress([&]() -> void { BPMSelector(); });
    sequencerMenu.AddUIComponent(&bpmSelectorBtn, Point(0, 2));

    UIButton systemSettingBtn;
    systemSettingBtn.SetName("System Setting");
    systemSettingBtn.SetColor(Color::White);
    systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
    sequencerMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

    sequencerMenu.AllowExit(false);
    sequencerMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
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
        return false;
    });

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
    trackSelector.OnChange([&](uint8_t val) -> void {
        if (sequence.GetChannel(track) != channel) {
            sequence.SetChannel(track, channel);
        }

        track = this->track;
        channel = sequence.GetChannel(track);
        offsettedChannel = channel + 1;
    });
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
  channelInput.SetValuePointer((uint16_t*)&channel);
  channelInput.OnChange([&](uint16_t val) -> void {
    offsettedChannel = val + 1;
  });

  channelSelector.AddUIComponent(channelInput, Point(0, 6));

  channelSelector.SetPostRenderFunc([&]() -> void {
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
    }
  });

  channelSelector.Start();

  if (sequence.GetChannel(track) != channel) {
    sequence.SetChannel(track, channel);
  }
}

void Sequencer::BPMSelector()
{

}
