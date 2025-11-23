#include "ControlBar.h"
#include "NotePad.h"

ControlBar::ControlBar(Sequencer* sequencer, SequencerNotePad* notePad)
{
    this->sequencer = sequencer;
    this->notePad = notePad;
}

void ControlBar::OnClear(std::function<void()> callback)
{
    clearCallback = callback;
}

Dimension ControlBar::GetSize() { return Dimension(8, 1); }

bool ControlBar::KeyEvent(Point xy, KeyInfo* keyInfo)
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

            if(sequencer->currentView == Sequencer::ViewMode::StepDetail)
            {
              sequencer->SetView(Sequencer::ViewMode::Sequencer);
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
          if(sequencer->ShiftActive())
          {
            sequencer->patternView = !sequencer->patternView;
            sequencer->ShiftEventOccured();
          }

          if(sequencer->currentView == Sequencer::ViewMode::Sequencer && sequencer->stepSelected.size() == 1)
          {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
            uint8_t step = *sequencer->stepSelected.begin();

            sequencer->ShiftEventOccured();
            sequencer->sequence.SetPosition(track, clip, patternIdx, step);
            sequencer->SetView(Sequencer::ViewMode::StepDetail);
            sequencer->ClearActiveNotes();
            sequencer->ClearSelectedNotes();
            sequencer->stepSelected.clear();
          }

          sequencer->shiftOnTime = MatrixOS::SYS::Millis();
          sequencer->shift[0] = true;
        }
        else if(keyInfo->state == RELEASED)
        {
          if(sequencer->meta.tracks[track].mode != SequenceTrackMode::NoteTrack)
          {
            // No octave
          }
          else if(keyInfo->hold == false && !sequencer->shiftEventOccured[0] && !sequencer->shiftEventOccured[1])
          {
            if(sequencer->currentView != Sequencer::ViewMode::Sequencer)
            {
              sequencer->currentView = Sequencer::ViewMode::Sequencer;
            }
            else if(sequencer->meta.tracks[track].mode != SequenceTrackMode::NoteTrack)
            {
              // No octave
            }
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
          sequencer->shift[0] = false;
          sequencer->shiftEventOccured[0] = false;
        }
        return true;
    }
    // Octave Up (x=7)
    else if(xy.x == 7)
    {
        if(keyInfo->state == PRESSED)
        {
          if(sequencer->ShiftActive())
          {
            sequencer->patternView = !sequencer->patternView;
            sequencer->ShiftEventOccured();
          }

          if(sequencer->currentView == Sequencer::ViewMode::Sequencer && sequencer->stepSelected.size() == 1)
          {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
            uint8_t step = *sequencer->stepSelected.begin();

            sequencer->ShiftEventOccured();
            sequencer->sequence.SetPosition(track, clip, patternIdx, step);
            sequencer->SetView(Sequencer::ViewMode::StepDetail);
            sequencer->ClearActiveNotes();
            sequencer->ClearSelectedNotes();
            sequencer->stepSelected.clear();
          }

          sequencer->shiftOnTime = MatrixOS::SYS::Millis();
          sequencer->shift[1] = true;
        }
        else if(keyInfo->state == RELEASED)
        {
          if(keyInfo->hold == false && !sequencer->shiftEventOccured[0] && !sequencer->shiftEventOccured[1])
          {
            if(sequencer->currentView != Sequencer::ViewMode::Sequencer)
            {
              sequencer->currentView = Sequencer::ViewMode::Sequencer;
            }
            else if(sequencer->meta.tracks[track].mode != SequenceTrackMode::NoteTrack)
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
          sequencer->shift[1] = false;
          sequencer->shiftEventOccured[1] = false;
        }
        return true;
    }
    return true;
}

Color ControlBar::GetOctavePlusColor() {
  Color color = sequencer->meta.tracks[sequencer->track].color;

  if(sequencer->currentView != Sequencer::ViewMode::Sequencer)
  {
    return color;
  }


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

Color ControlBar::GetOctaveMinusColor() {
  Color color = sequencer->meta.tracks[sequencer->track].color;

  if(sequencer->currentView != Sequencer::ViewMode::Sequencer)
  {
    return color;
  }

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

bool ControlBar::Render(Point origin)
{
  uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();

  // Play
  {
    Point point = origin + Point(0, 0);
    if(MatrixOS::KeyPad::GetKey(point)->Active())
    {
      MatrixOS::LED::SetColor(point, Color::White);
    }
    else if(sequencer->sequence.Playing())
    {
      uint8_t scale = breathingScale / 4 * 3;
      MatrixOS::LED::SetColor(point, Color::Crossfade(Color::Green, Color::White, Fract16(scale, 8)));
    }
    else
    {
      MatrixOS::LED::SetColor(point, Color::Green);
    }
  }

  // Record
  {
    Point point = origin + Point(1, 0);
    if(MatrixOS::KeyPad::GetKey(point)->Active())
    {
      MatrixOS::LED::SetColor(point, Color::White);
    }
    else if(sequencer->sequence.RecordEnabled())
    {
      uint8_t scale = breathingScale / 4 * 3 + 64;
      MatrixOS::LED::SetColor(point, Color::Red.Scale(scale));
    }
    else
    {
      MatrixOS::LED::SetColor(point, Color::Red);
    }
  }

  // Session View
  {
    Point point = origin + Point(2, 0);
    Color color;
    if(MatrixOS::KeyPad::GetKey(point)->Active())
    {
      color = Color::White;
    }
    else if(sequencer->currentView == Sequencer::ViewMode::Session)
    {
      color = Color(0xFFFFA0);
    }
    else
    {
      color = Color(0xFFFF00);
    }
    MatrixOS::LED::SetColor(point, color);
  }

  // Mixer View
  {
    Point point = origin + Point(3, 0);
    Color color;
    if(MatrixOS::KeyPad::GetKey(point)->Active())
    {
      color = Color::White;
    }
    else if(sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      color = Color(0xA0FFC0);
    }
    else
    {
      color = Color(0x00FF60);
    }
    MatrixOS::LED::SetColor(point, color);
  }

  // Clear
  {
    Point point = origin + Point(4, 0);
    Color color;
    if(sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      color = Color(0xFF0080).Dim();
    }
    else
    {
      color = MatrixOS::KeyPad::GetKey(point)->Active() ?
              Color::White :
              Color(0xFF0080);
    }
    MatrixOS::LED::SetColor(point, color);
  }

  // Copy
  {
    Point point = origin + Point(5, 0);
    Color color;
    if(sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      color = Color(0x0080FF).Dim();
    }
    else
    {
      color = MatrixOS::KeyPad::GetKey(point)->Active() ?
              Color::White :
              Color(0x0080FF);
    }
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
