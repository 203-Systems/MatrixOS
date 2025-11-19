#include "Sequencer.h"
#include "UI/UI.h"

#include "Scales.h"
#include "TrackSelector.h"


void Sequencer::Setup(const vector<string>& args)
{
    // Application initialization code here
    if(lastSequence.Get().empty())
    {
        sequence = Sequence(8);
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
    
}


void Sequencer::SequencerMenu()
{
    UI sequencerMenu("Sequencer Menu", Color(0x00FFFF), true);

    // UISelector trackSelector;
    // trackSelector.SetCount(8);
    // trackSelector.SetDimension(Dimension(8, 1));
    // trackSelector.SetIndividualColorFunc([&](uint16_t index) -> Color {
    //     if(index >= meta.tracks.size()) {return Color::Black;}
    //     return meta.tracks[index].color;
    // });
    // trackSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return "Track " + std::to_string(index + 1); });
    // trackSelector.SetValuePointer((uint16_t*)&track);
    // trackSelector.OnChange([&](uint16_t val) -> void {track = val;});
    // sequencerMenu.AddUIComponent(&trackSelector, Point(0, 0));

    // UIButton colorSelectorBtn;
    // colorSelectorBtn.SetName("Color Selector");
    // colorSelectorBtn.SetColorFunc([&]() -> Color {
    //     if(track < meta.tracks.size()) {
    //         return meta.tracks[track].color;
    //     }
    //     return Color::White;
    // });
    // colorSelectorBtn.OnPress([&]() -> void {
    //     if(track < meta.tracks.size() && MatrixOS::UIUtility::ColorPicker(meta.tracks[track].color)) {
    //         sequence.SetDirty();
    //     }
    // });
    // sequencerMenu.AddUIComponent(&colorSelectorBtn, Point(7, 2));

    TrackSelector trackSelector(&sequence, &meta, &track);
    sequencerMenu.AddUIComponent(&trackSelector, Point(0, 0));

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

    UIButton bpmSelectorBtn;
    bpmSelectorBtn.SetName("BPM Selector");
    bpmSelectorBtn.SetColor(Color(0xFF0080));
    bpmSelectorBtn.OnPress([&]() -> void { BPMSelector(); });
    sequencerMenu.AddUIComponent(&bpmSelectorBtn, Point(7, 5));

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
  UI channelSelector("Channel Selector", color, false);
  uint16_t channel = sequence.GetChannel(track);

  int32_t offsettedChannel = channel + 1;
  UI4pxNumber numDisplay;
  numDisplay.SetColor(color);
  numDisplay.SetDigits(2);
  numDisplay.SetValuePointer(&offsettedChannel);
  numDisplay.SetAlternativeColor(Color::White);
  numDisplay.SetSpacing(1);
  channelSelector.AddUIComponent(numDisplay, Point(1, 0));

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
    MatrixOS::LED::SetColor(Point(0, 0), color);
    MatrixOS::LED::SetColor(Point(0, 1), color);
    MatrixOS::LED::SetColor(Point(0, 2), color);
    MatrixOS::LED::SetColor(Point(0, 3), color);
    MatrixOS::LED::SetColor(Point(1, 0), color);
    MatrixOS::LED::SetColor(Point(1, 3), color);

    if(channel < 9)
    {
      //h
      MatrixOS::LED::SetColor(Point(2, 0), Color::White);
      MatrixOS::LED::SetColor(Point(2, 1), Color::White);
      MatrixOS::LED::SetColor(Point(2, 2), Color::White);
      MatrixOS::LED::SetColor(Point(2, 3), Color::White);
      MatrixOS::LED::SetColor(Point(3, 1), Color::White);
      MatrixOS::LED::SetColor(Point(4, 0), Color::White);
      MatrixOS::LED::SetColor(Point(4, 1), Color::White);
      MatrixOS::LED::SetColor(Point(4, 2), Color::White);
      MatrixOS::LED::SetColor(Point(4, 3), Color::White);
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
