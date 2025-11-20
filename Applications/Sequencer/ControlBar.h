#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class SequencerNotePad; // Forward declaration

class ControlBar : public UIComponent {
    Sequencer* sequencer;
    SequencerNotePad* notePad;

    public:
    ControlBar(Sequencer* sequencer, SequencerNotePad* notePad)
    {
        this->sequencer = sequencer;
        this->notePad = notePad;
    }

    Dimension GetSize() { return Dimension(8, 1); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        if(keyInfo->state == PRESSED)
        {
            uint8_t track = sequencer->track;

            // Octave Down (x=6)
            if(xy.x == 6)
            {
                if(sequencer->meta.tracks[track].config.note.octave > 0)
                {
                    sequencer->meta.tracks[track].config.note.octave--;
                    sequencer->sequence.SetDirty();
                    if(notePad != nullptr)
                    {
                        notePad->GenerateKeymap();
                    }
                }
                return true;
            }
            // Octave Up (x=7)
            else if(xy.x == 7)
            {
                if(sequencer->meta.tracks[track].config.note.octave < 9)
                {
                    sequencer->meta.tracks[track].config.note.octave++;
                    sequencer->sequence.SetDirty();
                    if(notePad != nullptr)
                    {
                        notePad->GenerateKeymap();
                    }
                }
                return true;
            }
        }
        return true;
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
                Color::Green;//.DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color); 
      }

      // Record
      {
        Point point = origin + Point(1, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color::Red; // .DimIfNot(sequencer->sequence.Playing());
        MatrixOS::LED::SetColor(point, color);
      }

      // Mixer View
      {
        Point point = origin + Point(2, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color(0xFFFF00);
        MatrixOS::LED::SetColor(point, color);
      }

      // ???
      {
        Point point = origin + Point(3, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color(0x00FF40);
        MatrixOS::LED::SetColor(point, color);
      }

      // Clear
      {
        Point point = origin + Point(4, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color(0xFF0080);
        MatrixOS::LED::SetColor(point, color);
      }

      // Copy
      {
        Point point = origin + Point(5, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                Color(0x00C0FF);
        MatrixOS::LED::SetColor(point, color);
      }

      // Octave -
      {
        Point point = origin + Point(6, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                GetOctaveMinusColor();
        MatrixOS::LED::SetColor(point, color);
      }

      // Octave +
      {
        Point point = origin + Point(7, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() ?
                Color::White :
                GetOctavePlusColor();
        MatrixOS::LED::SetColor(point, color);
      }
      return true;
    }
};
