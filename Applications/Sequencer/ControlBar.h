#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class ControlBar : public UIComponent {
    Sequencer* sequencer;

    public:
    ControlBar(Sequencer* sequencer)
    {
        this->sequencer = sequencer;
    }

    Dimension GetSize() { return Dimension(8, 1); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        return false;
    }

    const uint8_t OctaveGradient[8]  = {0, 16, 42, 68, 124, 182, 255};
    Color GetOctavePlusColor() {
      int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
      Color color = sequencer->meta.tracks[sequencer->track].color;
      uint8_t brightness;

      if (octave >= 4) {
          brightness = 255;
      } else {
          // Use gradient for octaves below 4 - dimmer as it goes down
          uint8_t index = (4 - octave) > 6 ? 6 : (4 - octave);
          brightness = OctaveGradient[6 - index];
      }

      return color.Scale(brightness);
}

  Color GetOctaveMinusColor() {
      int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
      Color color = sequencer->meta.tracks[sequencer->track].color;
      uint8_t brightness;

      if (octave <= 4) {
          brightness = 255;
      } else {
          // Use gradient for octaves above 4 - dimmer as it goes up
          uint8_t index = (octave - 4) > 6 ? 6 : (octave - 4);
          brightness = OctaveGradient[6 - index];
      }

      return color.Scale(brightness);
  }

    virtual bool Render(Point origin)
    {
      // Play
      {
        Point point = origin + Point(0, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color::Green.DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color); 
      }

      // Record
      {
        Point point = origin + Point(1, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color::Red.DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color); 
      }

      // Octave +
      {
        Point point = origin + Point(6, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                GetOctavePlusColor().DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color); 
      }

      // Octave -
      {
        Point point = origin + Point(7, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                GetOctaveMinusColor().DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color); 
      }
      return true;
    }
};
