#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class SequencerNotePad; // Forward declaration

class ControlBar : public UIComponent {
    Sequencer* sequencer;
    SequencerNotePad* notePad;
    std::function<void()> clearCallback;

    public:
    ControlBar(Sequencer* sequencer, SequencerNotePad* notePad)
    {
        this->sequencer = sequencer;
        this->notePad = notePad;
    }

    void OnClear(std::function<void()> callback)
    {
        clearCallback = callback;
    }

    Dimension GetSize() { return Dimension(8, 1); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        uint8_t track = sequencer->track;

        // Play (x=0)
        if(xy.x == 0)
        {
            if(keyInfo->state == PRESSED)
            {
              if(sequencer->sequence.Playing())
              {
                sequencer->sequence.Stop();
              }
              else
              {
                if(sequencer->ShiftActive())
                {
                  sequencer->ShiftEventOccured();
                  sequencer->sequence.Play(sequencer->track);
                }
                else
                {
                  sequencer->sequence.Play();
                }
              }
            }
            return true;
        }
        // Record (x=1)
        else if(xy.x == 1)
        {
            if(keyInfo->state == PRESSED)
            {
              sequencer->sequence.EnableRecord(!sequencer->sequence.RecordEnabled());
            }
            return true;
        }
        // Session (x=2)
        else if(xy.x == 2)
        {
            if(keyInfo->state == PRESSED)
            {
              if(sequencer->currentView == Sequencer::ViewMode::Session)
              {
                sequencer->SetView(Sequencer::ViewMode::Sequencer);
              }
              else
              {
                sequencer->SetView(Sequencer::ViewMode::Session);
              }
            }
            else if(keyInfo->state == RELEASED && keyInfo->Hold() && sequencer->currentView == Sequencer::ViewMode::Session)
            {
              sequencer->SetView(Sequencer::ViewMode::Sequencer);
            }
            return true;
        }
        // Mixer (x=3)
        else if(xy.x == 3)
        {
            if(keyInfo->state == PRESSED)
            {
              if(sequencer->currentView == Sequencer::ViewMode::Mixer)
              {
                sequencer->SetView(Sequencer::ViewMode::Sequencer);
              }
              else
              {
                sequencer->SetView(Sequencer::ViewMode::Mixer);
              }
            }
            else if(keyInfo->state == RELEASED && keyInfo->Hold() && sequencer->currentView == Sequencer::ViewMode::Mixer)
            {
              sequencer->SetView(Sequencer::ViewMode::Sequencer);
            }
            return true;
        }
        // Clear (x=4)
        else if(xy.x == 4)
        {
            if(keyInfo->state == PRESSED)
            {
                sequencer->clear = true;
            }
            else if(keyInfo->state == RELEASED)
            {
                sequencer->clear = false;
                if(clearCallback != nullptr)
                {
                    clearCallback();
                }
            }
            return true;
        }
        // Copy (x=5)
        else if(xy.x == 5)
        {
            if(keyInfo->state == PRESSED)
            {
                sequencer->copy = true;
            }
            else if(keyInfo->state == RELEASED)
            {
                sequencer->copy = false;
            }
            return true;
        }
        // Octave Down (x=6)
        else if(xy.x == 6)
        {
            if(keyInfo->state == PRESSED)
            {
              if(sequencer->shift)
              {
                sequencer->patternView = !sequencer->patternView;
                sequencer->shiftEventOccured = true;
              }
              sequencer->shiftOnTime = MatrixOS::SYS::Millis();
              sequencer->shift++;
            }
            else if(keyInfo->state == RELEASED)
            {
              if(sequencer->meta.tracks[track].mode != SequenceTrackMode::NoteTrack)
              {
                // No octave
              }
              else if(keyInfo->hold == false && sequencer->shiftEventOccured == false)
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
              }
              sequencer->shift--;

              if(sequencer->shift == 0)
              {
                sequencer->shiftEventOccured = false;
              }
            }
            return true;
        }
        // Octave Up (x=7)
        else if(xy.x == 7)
        {
            if(keyInfo->state == PRESSED)
            {
              if(sequencer->shift)
              {
                sequencer->patternView = !sequencer->patternView;
                sequencer->shiftEventOccured = true;
              }
              sequencer->shiftOnTime = MatrixOS::SYS::Millis();
              sequencer->shift++;
            }
            else if(keyInfo->state == RELEASED)
            {
              if(keyInfo->hold == false && sequencer->shiftEventOccured == false)
              {
                if(sequencer->meta.tracks[track].mode != SequenceTrackMode::NoteTrack)
                {
                  // No octave
                }
                else if(sequencer->meta.tracks[track].config.note.octave < 9)
                {
                    sequencer->meta.tracks[track].config.note.octave++;
                    sequencer->sequence.SetDirty();
                    if(notePad != nullptr)
                    {
                        notePad->GenerateKeymap();
                    }
                }
              }
              sequencer->shift--;

              if(sequencer->shift == 0)
              {
                sequencer->shiftEventOccured = false;
              }
            }
            return true;
        }
        return true;
    }

    const uint8_t OctaveGradient[8]  = {0, 16, 42, 68, 124, 182, 255};
    Color GetOctavePlusColor() {
      Color color = sequencer->meta.tracks[sequencer->track].color;

      if(sequencer->meta.tracks[sequencer->track].mode != SequenceTrackMode::NoteTrack)
      {
        return color;
      }

      int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
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
      Color color = sequencer->meta.tracks[sequencer->track].color;

      if(sequencer->meta.tracks[sequencer->track].mode != SequenceTrackMode::NoteTrack)
      {
        return color;
      }

      int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
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

      // Session View
      {
        Point point = origin + Point(2, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() || sequencer->currentView == Sequencer::ViewMode::Session ?
                Color::White :
                Color(0xFFFF00);
        MatrixOS::LED::SetColor(point, color);
      }

      // Mixer View
      {
        Point point = origin + Point(3, 0);
        Color color =  MatrixOS::KeyPad::GetKey(point)->Active() || sequencer->currentView == Sequencer::ViewMode::Mixer ?
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
                Color(0x00A0FF);
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
