#include "ControlBar.h"
#include "NotePad.h"
#include <algorithm>

SequencerControlBar::SequencerControlBar(Sequencer* sequencer, SequencerNotePad* notePad)
{
    this->sequencer = sequencer;
    this->notePad = notePad;
}

Dimension SequencerControlBar::GetSize() { return Dimension(8, 1); }

bool SequencerControlBar::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    bool nudgeEnabled = !sequencer->sequence.Playing(sequencer->track) && !sequencer->stepSelected.empty();
    switch (xy.x)
    {
    case 0: return nudgeEnabled ? HandleNudgeKey(false, keyInfo) : HandlePlayKey(keyInfo);
    case 1: return nudgeEnabled ? HandleNudgeKey(true, keyInfo) : HandleRecordKey(keyInfo);
    case 2: return HandleSessionKey(keyInfo);
    case 3: return HandleMixerKey(keyInfo);
    case 4: return HandleClearKey(keyInfo);
    case 5: return HandleCopyKey(keyInfo);
    case 6: return HandleOctaveKey(0, /*up*/false, keyInfo);
    case 7: return HandleOctaveKey(1, /*up*/true, keyInfo);
    default: return true;
    }
}

bool SequencerControlBar::HandlePlayKey(KeyInfo* keyInfo)
{
    if(keyInfo->state == PRESSED)
    {
      if((sequencer->CopyActive() || sequencer->ClearActive()) && sequencer->sequence.Playing() == false)
      {
        return true; // Disable play if we have copy or clear active
      }
      else if(sequencer->ShiftActive())
      {
        sequencer->ShiftEventOccured();
        bool trackPlaying = sequencer->sequence.Playing(sequencer->track);
        if(trackPlaying)
        {
          sequencer->sequence.StopAfter(sequencer->track);
        }
        else
        {
          sequencer->sequence.Play(sequencer->track);
          if(sequencer->currentView == Sequencer::ViewMode::StepDetail)
          {
            sequencer->SetView(Sequencer::ViewMode::Sequencer);
          }
        }
      }
      else
      {
        if(sequencer->sequence.Playing())
        {
          sequencer->sequence.Stop();
        }
        else
        {
          sequencer->sequence.Play();
          if(sequencer->currentView == Sequencer::ViewMode::StepDetail)
          {
            sequencer->SetView(Sequencer::ViewMode::Sequencer);
          }
        }
      }
    }
    return true;
}

bool SequencerControlBar::HandleRecordKey(KeyInfo* keyInfo)
{
    if(keyInfo->state == PRESSED)
    {
      if(sequencer->ShiftActive() && !sequencer->sequence.Playing())
      {
        sequencer->ShiftEventOccured();
        sequencer->sequence.UndoLastRecorded();
        return true;
      }
      sequencer->sequence.EnableRecord(!sequencer->sequence.RecordEnabled());
    }
    return true;
}

bool SequencerControlBar::HandleSessionKey(KeyInfo* keyInfo)
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

bool SequencerControlBar::HandleMixerKey(KeyInfo* keyInfo)
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

bool SequencerControlBar::HandleClearKey(KeyInfo* keyInfo)
{
    if(keyInfo->state == PRESSED)
    {
        sequencer->clear = true;
        if(sequencer->trackSelected)
        {
          sequencer->sequence.ClearAllStepsInClip(sequencer->track, sequencer->sequence.GetPosition(sequencer->track).clip);
        }
    }
    else if(keyInfo->state == RELEASED)
    {
        sequencer->clear = false;
        if(sequencer->stepSelected.empty() == false)
        {
            uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
            SequencePattern* pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
            if (!pattern) { return true; }
            for (const auto& step : sequencer->stepSelected)
            {
                sequencer->sequence.PatternClearStepEvents(pattern, step, pulsesPerStep);
            }
        }
    }
    return true;
}

bool SequencerControlBar::HandleCopyKey(KeyInfo* keyInfo)
{
    if(keyInfo->state == PRESSED)
    {
        sequencer->copy = true;
        if(sequencer->sequence.Playing(sequencer->track) == false)
        {
          if(sequencer->stepSelected.size() == 1)
          {
            sequencer->copySourceStep = *sequencer->stepSelected.begin();
          }
          else
          {
            sequencer->copySourceStep = -1;
          }
          sequencer->stepSelected.clear();
        }
    }
    else if(keyInfo->state == RELEASED)
    {
        sequencer->copy = false;
    }
    return true;
}

bool SequencerControlBar::HandleNudgeKey(bool positive, KeyInfo* keyInfo)
{
    if (keyInfo->state != RELEASED) { return true; }

    uint8_t track = sequencer->track;
    if (sequencer->sequence.Playing(track)) { return true; }
    if (sequencer->stepSelected.empty()) { return true; }

    SequencePosition pos = sequencer->sequence.GetPosition(track);
    SequencePattern* pattern = sequencer->sequence.GetPattern(track, pos.clip, pos.pattern);
    if (!pattern) { return true; }

    uint16_t base = sequencer->sequence.GetPulsesPerStep();
    uint16_t step = base;
    int16_t offset = positive ? step : - (int16_t)step;

    sequencer->sequence.PatternNudge(pattern, offset);
    return true;
}

bool SequencerControlBar::HandleOctaveKey(uint8_t idx, bool up, KeyInfo* keyInfo)
{
    uint8_t track = sequencer->track;
    uint8_t shiftIndex = up ? 1 : 0;

    if(keyInfo->state == PRESSED)
    {
      if(sequencer->ShiftActive())
      {
        sequencer->patternView = !sequencer->patternView;
        sequencer->ShiftEventOccured();
      }

      if(sequencer->currentView == Sequencer::ViewMode::Sequencer && sequencer->stepSelected.size() == 1)
      {
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
      sequencer->shift[shiftIndex] = true;
    }
    else if(keyInfo->state == RELEASED)
    {
      bool shiftEvent = sequencer->shiftEventOccured[0] || sequencer->shiftEventOccured[1];
      if(keyInfo->hold == false && !shiftEvent)
      {
        if(sequencer->currentView != Sequencer::ViewMode::Sequencer)
        {
          sequencer->currentView = Sequencer::ViewMode::Sequencer;
        }
        else if(sequencer->meta.tracks[track].mode == SequenceTrackMode::NoteTrack)
        {
          auto& noteCfg = sequencer->meta.tracks[track].config.note;
          if(up)
          {
            if(noteCfg.octave < 9)
            {
                noteCfg.octave++;
                sequencer->sequence.SetDirty();
                if(notePad != nullptr) { notePad->GenerateKeymap(); }
            }
          }
          else
          {
            if(noteCfg.octave > 0)
            {
              noteCfg.octave--;
              sequencer->sequence.SetDirty();
              if(notePad != nullptr) { notePad->GenerateKeymap(); }
            }
          }
        }
      }
      sequencer->shift[shiftIndex] = false;
      sequencer->shiftEventOccured[shiftIndex] = false;
    }
    return true;
}

Color SequencerControlBar::GetOctavePlusColor() {
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

Color SequencerControlBar::GetOctaveMinusColor() {
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

bool SequencerControlBar::Render(Point origin)
{
  uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();
  bool nudgeEnabled = !sequencer->sequence.Playing(sequencer->track) && !sequencer->stepSelected.empty();

  // Play (or Nudge Left)
  {
    Point point = origin + Point(0, 0);
    if (nudgeEnabled)
    {
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF8000);
      MatrixOS::LED::SetColor(point, color);
    }
    else
    {
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
  }

  // Record (or Nudge Right)
  {
    Point point = origin + Point(1, 0);
    if (nudgeEnabled)
    {
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF8000);
      MatrixOS::LED::SetColor(point, color);
    }
    else
    {
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
