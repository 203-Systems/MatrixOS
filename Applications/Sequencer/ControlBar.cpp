#include "ControlBar.h"
#include "NotePad.h"
#include <algorithm>

SequencerControlBar::SequencerControlBar(Sequencer *sequencer, SequencerNotePad *notePad)
{
  this->sequencer = sequencer;
  this->notePad = notePad;
}

Dimension SequencerControlBar::GetSize() { return Dimension(8, 1); }

bool SequencerControlBar::KeyEvent(Point xy, KeyInfo *keyInfo)
{
  bool stepSelected = !sequencer->stepSelected.empty();
  bool patternSelected = !sequencer->patternSelected.empty();


  if (stepSelected)
  {
    switch (xy.x)
    {
      case 0:
        return HandleStepOctaveOffsetKey(false, keyInfo);
      case 1:
        return HandleStepOctaveOffsetKey(true, keyInfo);
      case 2:
        return HandleQuantizeKey(keyInfo);
      case 3:
        return HandleTwoPatternToggleKey(keyInfo);
    }
  }
  else if (patternSelected)
  {
    switch (xy.x)
    {
      case 0:
        return HandleOctaveOffsetKey(false, keyInfo);
      case 1:
        return HandleOctaveOffsetKey(true, keyInfo);
      case 2:
        return HandleNudgeKey(false, keyInfo);
      case 3:
        return HandleNudgeKey(true, keyInfo);
    }
  }

  switch (xy.x)
  {
  case 0:
    return HandlePlayKey(keyInfo);
  case 1:
    return HandleRecordKey(keyInfo);
  case 2:
    return HandleSessionKey(keyInfo);
  case 3:
    return HandleMixerKey(keyInfo);
  case 4:
    return HandleClearKey(keyInfo);
  case 5:
    return HandleCopyKey(keyInfo);
  case 6:
    return HandleShiftKey(0, /*right*/ false, keyInfo);
  case 7:
    return HandleShiftKey(1, /*right*/ true, keyInfo);
  default:
    return true;
  }
}

bool SequencerControlBar::HandlePlayKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::PLAY, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::PLAY)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    if (sequencer->ShiftActive())
    {
      sequencer->ShiftEventOccured();
      bool trackPlaying = sequencer->sequence.Playing(sequencer->track);
      if (trackPlaying)
      {
        sequencer->sequence.StopAfter(sequencer->track);
      }
      else
      {
        sequencer->sequence.Play(sequencer->track);
        if (sequencer->currentView == Sequencer::ViewMode::StepDetail)
        {
          sequencer->SetView(Sequencer::ViewMode::Sequencer);
        }
      }
    }
    else
    {
      if (sequencer->sequence.Playing())
      {
        sequencer->sequence.Stop();
      }
      else
      {
        sequencer->sequence.Play();
        if (sequencer->currentView == Sequencer::ViewMode::StepDetail)
        {
          sequencer->SetView(Sequencer::ViewMode::Sequencer);
        }
      }
    }
  }
  return true;
}

bool SequencerControlBar::HandleRecordKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(sequencer->ClearActive() ? SequencerMessage::UNDO : SequencerMessage::RECORD, true);
  }
  else if (keyInfo->state == RELEASED && (sequencer->lastMessage == SequencerMessage::RECORD || sequencer->lastMessage == SequencerMessage::UNDO))
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    if (sequencer->ClearActive())
    {
      if(!sequencer->sequence.Playing())
      {
        sequencer->sequence.UndoLastRecorded();
        sequencer->SetMessage(SequencerMessage::UNDONE);
      }
    }
    else
    { 
      sequencer->sequence.EnableRecord(!sequencer->sequence.RecordEnabled());
    }
  }
  return true;
}

bool SequencerControlBar::HandleSessionKey(KeyInfo *keyInfo)
{

  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::CLIP, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::CLIP)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    if (sequencer->currentView == Sequencer::ViewMode::Session)
    {
      sequencer->SetView(Sequencer::ViewMode::Sequencer);
    }
    else
    {
      sequencer->SetView(Sequencer::ViewMode::Session);
    }
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() && sequencer->currentView == Sequencer::ViewMode::Session)
  {
    sequencer->SetView(Sequencer::ViewMode::Sequencer);
  }
  return true;
}

bool SequencerControlBar::HandleMixerKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::MIX, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::MIX)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    if (sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      sequencer->SetView(Sequencer::ViewMode::Sequencer);
    }
    else
    {
      sequencer->SetView(Sequencer::ViewMode::Mixer);
    }
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() && sequencer->currentView == Sequencer::ViewMode::Mixer)
  {
    sequencer->SetView(Sequencer::ViewMode::Sequencer);
  }
  return true;
}

bool SequencerControlBar::HandleClearKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == PRESSED)
  {
    sequencer->clear = true;
    sequencer->copy = false;

    SequencePosition* pos = sequencer->sequence.GetPosition(sequencer->track);

    if (sequencer->activeTrackSelected)
    {
      sequencer->sequence.ClearAllStepsInClip(sequencer->track, pos->clip);
      sequencer->SetMessage(SequencerMessage::CLEARED);
    }

    if (sequencer->stepSelected.empty() == false)
    {
      uint8_t track = sequencer->track;
      uint8_t clip = pos->clip;
      uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
      for (const auto &selection : sequencer->stepSelected)
      {
        uint8_t patternIdx = selection.first;
        uint8_t step = selection.second;
        SequencePattern *pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
        if (!pattern)
        {
          return true;
        }
        // Remove notes from noteActive and send noteOff

        uint16_t startTime = step * pulsesPerStep;
        uint16_t endTime = startTime + pulsesPerStep - 1;
        uint8_t channel = sequencer->sequence.GetChannel(track);
        auto it = pattern->events.lower_bound(startTime);
        while (it != pattern->events.end() && it->first <= endTime)
        {
          if (it->second.eventType == SequenceEventType::NoteEvent)
          {
            const SequenceEventNote &noteData = std::get<SequenceEventNote>(it->second.data);
            auto activeIt = sequencer->noteActive.find(noteData.note);
            if (activeIt != sequencer->noteActive.end())
            {
              MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0));
              sequencer->noteActive.erase(activeIt);
            }
          }
          ++it;
        }
        
        if(sequencer->sequence.PatternClearStepEvents(pattern, step, pulsesPerStep))
        {
          sequencer->SetMessage(SequencerMessage::CLEARED);
        }
      }
    }

    if (sequencer->patternSelected.empty() == false)
    {
      uint8_t track = sequencer->track;
      uint8_t clip = pos->clip;

      // Create a vector from the set and sort in reverse order
      std::vector<uint8_t> patternsToDelete(sequencer->patternSelected.begin(), sequencer->patternSelected.end());
      std::sort(patternsToDelete.rbegin(), patternsToDelete.rend());

      // Delete patterns from last to first to avoid index shifting
      for (uint8_t patternIdx : patternsToDelete)
      {
        sequencer->sequence.DeletePattern(track, clip, patternIdx);
      }

      // Update current pattern index if needed
      uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
      uint8_t currentPattern = pos->pattern;
      if (currentPattern >= patternCount)
      {
        currentPattern = patternCount > 0 ? patternCount - 1 : 0;
        sequencer->sequence.SetPattern(track, currentPattern);
      }

      sequencer->ClearActiveNotes();
      sequencer->stepSelected.clear();
      sequencer->patternSelected.clear();
      sequencer->SetMessage(SequencerMessage::CLEARED);
    }
  }
  else if (keyInfo->state == RELEASED)
  {
    sequencer->clear = false;
  }
  return true;
}

bool SequencerControlBar::HandleCopyKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == PRESSED)
  {
    sequencer->copy = true;
    sequencer->clear = false;
    sequencer->copySource.Clear();
    sequencer->stepSelected.clear();
    sequencer->patternSelected.clear();
  }
  else if (keyInfo->state == RELEASED)
  {
    sequencer->copy = false;
  }
  return true;
}

bool SequencerControlBar::HandleNudgeKey(bool positive, KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::NUDGE, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::NUDGE)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    uint8_t track = sequencer->track;

    SequencePosition* pos = sequencer->sequence.GetPosition(track);
    SequencePattern *pattern = sequencer->sequence.GetPattern(track, pos->clip, pos->pattern);

    if (!pattern)
    {
      return true;
    }

    uint16_t base = sequencer->sequence.GetPulsesPerStep();
    uint16_t step = base;
    int16_t offset = positive ? step : -(int16_t)step;

    bool twoPatternMode = sequencer->meta.tracks[track].twoPatternMode;
    if (twoPatternMode == false)
    {
      sequencer->sequence.PatternNudge(pattern, offset);
    }
    else
    {
      SequencePattern *patternNext = sequencer->sequence.GetPattern(track, pos->clip, pos->pattern + 1);
      sequencer->sequence.DualPatternNudge(pattern, patternNext, offset);
    }
    sequencer->ClearActiveNotes();
  }
  return true;
}

bool SequencerControlBar::HandleOctaveOffsetKey(bool positive, KeyInfo* keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(positive ? SequencerMessage::OCTAVE_PLUS : SequencerMessage::OCTAVE_MINUS, true);
  }
  else if (keyInfo->state == RELEASED && (sequencer->lastMessage == SequencerMessage::OCTAVE_PLUS || sequencer->lastMessage == SequencerMessage::OCTAVE_MINUS))
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    uint8_t track = sequencer->track;
    SequencePosition* pos = sequencer->sequence.GetPosition(track);
    bool twoPatternMode = sequencer->meta.tracks[track].twoPatternMode;

    uint8_t pattern1Idx;
    uint8_t pattern2Idx;
    if (twoPatternMode == false)
    {
      pattern1Idx = pos->pattern;
      pattern2Idx = 255;
    }
    else
    {
      pattern1Idx = pos->pattern / 2 * 2;
      pattern2Idx = pattern1Idx + 1;
    }

    SequencePattern* pattern1 = sequencer->sequence.GetPattern(track, pos->clip, pattern1Idx);
    if (!pattern1)
    {
      return true;
    }

    int8_t offset = positive ? 12 : -12;
    uint16_t pattern1LengthPulses = pattern1->steps * sequencer->sequence.GetPulsesPerStep();

    // Apply octave offset to all notes in pattern 1
    sequencer->sequence.PatternOffsetNotesInRange(pattern1, 0, pattern1LengthPulses - 1, offset);

    // Apply to pattern 2 if in two-pattern mode
    if (twoPatternMode && pattern2Idx != 255)
    {
      SequencePattern* pattern2 = sequencer->sequence.GetPattern(track, pos->clip, pattern2Idx);
      if (pattern2)
      {
        uint16_t pattern2LengthPulses = pattern2->steps * sequencer->sequence.GetPulsesPerStep();
        sequencer->sequence.PatternOffsetNotesInRange(pattern2, 0, pattern2LengthPulses - 1, offset);
      }
    }

    sequencer->ClearActiveNotes();
    // sequencer->SetMessage(positive ? SequencerMessage::OCTAVE_PLUS_DONE : SequencerMessage::OCTAVE_MINUS_DONE);
  }
  return true;
}

bool SequencerControlBar::HandleStepOctaveOffsetKey(bool positive, KeyInfo* keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(positive ? SequencerMessage::OCTAVE_PLUS : SequencerMessage::OCTAVE_MINUS, true);
  }
  else if (keyInfo->state == RELEASED && (sequencer->lastMessage == SequencerMessage::OCTAVE_PLUS || sequencer->lastMessage == SequencerMessage::OCTAVE_MINUS))
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    uint8_t track = sequencer->track;
    SequencePosition* pos = sequencer->sequence.GetPosition(track);

    if(!sequencer->stepSelected.empty())
    {
      uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
      int8_t offset = positive ? 12 : -12;

      // Iterate through all selected steps
      for (const auto& [patternIdx, stepIdx] : sequencer->stepSelected)
      {
        SequencePattern* targetPattern = sequencer->sequence.GetPattern(track, pos->clip, patternIdx);
        if (!targetPattern) continue;

        // Calculate time range for this step
        uint16_t startTime = stepIdx * pulsesPerStep;
        uint16_t endTime = startTime + pulsesPerStep - 1;

        uint8_t channel = sequencer->sequence.GetChannel(track);

        // Send note-off for all notes in this step before transposing and remove from noteActive
        for (auto it = targetPattern->events.lower_bound(startTime); it != targetPattern->events.end() && it->first <= endTime; ++it)
        {
          if (it->second.eventType == SequenceEventType::NoteEvent)
          {
            SequenceEventNote noteData = std::get<SequenceEventNote>(it->second.data);
            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0), MIDI_PORT_ALL);

            // Remove from noteActive
            auto activeIt = sequencer->noteActive.find(noteData.note);
            if (activeIt != sequencer->noteActive.end())
            {
              sequencer->noteActive.erase(activeIt);
            }
          }
        }

        // Apply octave offset to all notes in this step
        sequencer->sequence.PatternOffsetNotesInRange(targetPattern, startTime, endTime, offset);

        // Send note-on for all transposed notes in this step and add to noteActive
        for (auto it = targetPattern->events.lower_bound(startTime); it != targetPattern->events.end() && it->first <= endTime; ++it)
        {
          if (it->second.eventType == SequenceEventType::NoteEvent)
          {
            SequenceEventNote noteData = std::get<SequenceEventNote>(it->second.data);
            MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity), MIDI_PORT_ALL);

            // Add to noteActive
            sequencer->noteActive.insert(noteData.note);
          }
        }
      }
      // sequencer->SetMessage(positive ? SequencerMessage::OCTAVE_PLUS_DONE : SequencerMessage::OCTAVE_MINUS_DONE);
    }
  }
  return true;
}

bool SequencerControlBar::HandleTwoPatternToggleKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::TWO_PATTERN_VIEW, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::TWO_PATTERN_VIEW)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    uint8_t track = sequencer->track;
    sequencer->meta.tracks[track].twoPatternMode = !sequencer->meta.tracks[track].twoPatternMode;
    sequencer->patternView = false; // So user know what happened.
    sequencer->stepSelected.clear();
    sequencer->patternSelected.clear();
  }
  return true;
}

bool SequencerControlBar::HandleQuantizeKey(KeyInfo *keyInfo)
{
  if (keyInfo->state == HOLD)
  {
    sequencer->SetMessage(SequencerMessage::QUANTIZE, true);
  }
  else if (keyInfo->state == RELEASED && sequencer->lastMessage == SequencerMessage::QUANTIZE)
  {
    sequencer->SetMessage(SequencerMessage::NONE);
  }
  else if (keyInfo->state == RELEASED && keyInfo->Hold() == false)
  {
    uint8_t track = sequencer->track;
    if (sequencer->sequence.Playing(track))
    {
      return true;
    }

    bool twoPatternMode = sequencer->meta.tracks[track].twoPatternMode;

    SequencePosition* pos = sequencer->sequence.GetPosition(track);

    uint8_t pattern1Idx;
    uint8_t pattern2Idx;
    uint8_t patternNextIdx;
    if (twoPatternMode == false)
    {
      pattern1Idx = pos->pattern;
      pattern2Idx = 255;
      patternNextIdx = (pattern1Idx + 1) >= sequencer->sequence.GetPatternCount(track, pos->clip) ? 0 : pattern1Idx + 1;
    }
    else
    {
      pattern1Idx = pos->pattern / 2 * 2;
      pattern2Idx = pattern1Idx + 1;
      patternNextIdx = (pattern2Idx + 1) >= sequencer->sequence.GetPatternCount(track, pos->clip) ? 0 : pattern2Idx + 1;
    }

    SequencePattern *pattern1 = sequencer->sequence.GetPattern(track, pos->clip, pattern1Idx);
    SequencePattern *pattern2 = sequencer->sequence.GetPattern(track, pos->clip, pattern2Idx); // Will return nullptr if pattern2Idx == 255
    SequencePattern *patternNext = sequencer->sequence.GetPattern(track, pos->clip, patternNextIdx);

    sequencer->sequence.DualPatternQuantize(pattern1, pattern2, patternNext, sequencer->sequence.GetPulsesPerStep());

    sequencer->SetMessage(SequencerMessage::QUANTIZED);
  }
  return true;
}

bool SequencerControlBar::HandleShiftKey(uint8_t idx, bool right, KeyInfo *keyInfo)
{
  uint8_t track = sequencer->track;
  uint8_t shiftIndex = right ? 1 : 0;

  if (keyInfo->state == PRESSED)
  {
    if (sequencer->ShiftActive())
    {
      if(sequencer->currentView == Sequencer::ViewMode::Sequencer)
      {
        sequencer->patternView = !sequencer->patternView;
      }
      else if(sequencer->currentView == Sequencer::ViewMode::Session)
      {
        sequencer->wideClipMode = !sequencer->wideClipMode;
        sequencer->clipWindow = 0;
      }
      sequencer->ShiftEventOccured();
    }

    sequencer->shiftOnTime = MatrixOS::SYS::Millis();
    sequencer->shift[shiftIndex] = true;
    sequencer->shiftEventOccured[shiftIndex] = false;

    if (sequencer->currentView == Sequencer::ViewMode::Sequencer && sequencer->stepSelected.size() == 1)
    {
      uint8_t clip = sequencer->sequence.GetPosition(track)->clip;
      auto selection = *sequencer->stepSelected.begin();
      uint8_t patternIdx = selection.first;
      uint8_t step = selection.second;
      SequencePattern *pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

      uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
      uint16_t startTime = step * pulsesPerStep;
      uint16_t endTime = startTime + pulsesPerStep - 1;

      if (pattern && sequencer->sequence.PatternHasEventInRange(pattern, startTime, endTime))
      {
        sequencer->ShiftEventOccured();
        sequencer->sequence.SetPosition(track, clip, patternIdx, step);
        sequencer->SetView(Sequencer::ViewMode::StepDetail);
      }
    }
  }
  else if (keyInfo->state == RELEASED)
  {
    if (sequencer->ShiftActive() == false)
    {
      return true;
    }

    bool shiftEvent = sequencer->shiftEventOccured[0] || sequencer->shiftEventOccured[1];
    if (keyInfo->hold == false && !shiftEvent)
    {
      if (sequencer->currentView == Sequencer::ViewMode::Session)
      {
        sequencer->clipWindow = shiftIndex;
      }
      else if (sequencer->currentView == Sequencer::ViewMode::Sequencer)
      {
        if (sequencer->meta.tracks[track].mode == SequenceTrackMode::NoteTrack)
        {
          auto &noteCfg = sequencer->meta.tracks[track].config.note;
          if (right)
          {
            if (noteCfg.octave < 9)
            {
              noteCfg.octave++;
              sequencer->sequence.SetDirty();
              if (notePad != nullptr)
              {
                notePad->GenerateKeymap();
              }
            }
          }
          else
          {
            if (noteCfg.octave > 0)
            {
              noteCfg.octave--;
              sequencer->sequence.SetDirty();
              if (notePad != nullptr)
              {
                notePad->GenerateKeymap();
              }
            }
          }
        }
      }
      else
      {
        sequencer->currentView = Sequencer::ViewMode::Sequencer;
      }
    }
    sequencer->shift[shiftIndex] = false;
    sequencer->shiftEventOccured[shiftIndex] = false;
  }
  return true;
}

Color SequencerControlBar::GetOctavePlusColor()
{
  Color color = sequencer->meta.tracks[sequencer->track].color;

  if (sequencer->meta.tracks[sequencer->track].mode != SequenceTrackMode::NoteTrack)
  {
    return color;
  }
  else if(!sequencer->stepSelected.empty())
  {
    return color;
  }

  int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
  uint8_t brightness;

  if (octave >= 4)
  {
    brightness = 255;
  }
  else
  {
    // Use gradient for octaves below 4 - dimmer as it goes down
    uint8_t index = (4 - octave) > 6 ? 6 : (4 - octave);
    brightness = OctaveGradient[6 - index];
  }

  return color.Scale(brightness);
}

Color SequencerControlBar::GetOctaveMinusColor()
{
  Color color = sequencer->meta.tracks[sequencer->track].color;

  if (sequencer->meta.tracks[sequencer->track].mode != SequenceTrackMode::NoteTrack)
  {
    return color;
  }
  else if(!sequencer->stepSelected.empty())
  {
    return color;
  }

  int8_t octave = sequencer->meta.tracks[sequencer->track].config.note.octave;
  uint8_t brightness;

  if (octave <= 4)
  {
    brightness = 255;
  }
  else
  {
    // Use gradient for octaves above 4 - dimmer as it goes right
    uint8_t index = (octave - 4) > 6 ? 6 : (octave - 4);
    brightness = OctaveGradient[6 - index];
  }

  return color.Scale(brightness);
}

bool SequencerControlBar::Render(Point origin)
{
  uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();
  bool patternSelected = !sequencer->patternSelected.empty();
  bool stepSelected = !sequencer->stepSelected.empty();

  if(stepSelected == false)
  {
    if(sequencer->lastMessage == SequencerMessage::NUDGE ||
       sequencer->lastMessage == SequencerMessage::QUANTIZE ||
       sequencer->lastMessage == SequencerMessage::TWO_PATTERN_VIEW)
    {
      sequencer->SetMessage(SequencerMessage::NONE);
    }
  }

  // Left 4 - Floating UI
  if (stepSelected) // Step Specific
  { 
    // Octave Up
    {
      Point point = origin + Point(0, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF0040);
      MatrixOS::LED::SetColor(point, color);
    }
    // Octave Down
    {
      Point point = origin + Point(1, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF0040);
      MatrixOS::LED::SetColor(point, color);
    }
    // Quantize
    {
      Point point = origin + Point(2, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0x00FF40);
      MatrixOS::LED::SetColor(point, color);
    }
    // 2 Pattern View
    {
      Point point = origin + Point(3, 0);
      uint8_t track = sequencer->track;
      bool twoPatternMode = sequencer->meta.tracks[track].twoPatternMode;
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFFFF00).DimIfNot(twoPatternMode);
      MatrixOS::LED::SetColor(point, color);
    }
  }
  else if (patternSelected) // Pattern Specific
  {
    // Octave Up
    {
      Point point = origin + Point(0, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF0040);
      MatrixOS::LED::SetColor(point, color);
    }
    // Octave Down
    {
      Point point = origin + Point(1, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF0040);
      MatrixOS::LED::SetColor(point, color);
    }
    // Nudge Left
    {
      Point point = origin + Point(2, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xA000FF);
      MatrixOS::LED::SetColor(point, color);
    }
    // Nudge Right
    {
      Point point = origin + Point(3, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xA000FF);
      MatrixOS::LED::SetColor(point, color);
    }
  }
  else // General
  {
    // Play
    {
      Point point = origin + Point(0, 0);
      if (MatrixOS::KeyPad::GetKey(point)->Active())
      {
        MatrixOS::LED::SetColor(point, Color::White);
      }
      else if (sequencer->sequence.Playing())
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
      if (MatrixOS::KeyPad::GetKey(point)->Active())
      {
        MatrixOS::LED::SetColor(point, Color::White);
      }
      else if (sequencer->ClearActive())
      {
        // Undo record
        MatrixOS::LED::SetColor(point, Color(0xFF0020).DimIfNot(sequencer->sequence.CanUndoLastRecord() && !sequencer->sequence.Playing()));
      }
      else if (sequencer->sequence.RecordEnabled())
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
      if (MatrixOS::KeyPad::GetKey(point)->Active())
      {
        color = Color::White;
      }
      else if (sequencer->currentView == Sequencer::ViewMode::Session)
      {
        color = Color(0xFFFFB3);
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
      if (MatrixOS::KeyPad::GetKey(point)->Active())
      {
        color = Color::White;
      }
      else if (sequencer->currentView == Sequencer::ViewMode::Mixer)
      {
        color = Color(0xC6FFB3);
      }
      else
      {
        color = Color(0x40FF00);
      }
      MatrixOS::LED::SetColor(point, color);
    }
  }

  // Right 4 - Constent UI
  // Clear
  {
    Point point = origin + Point(4, 0);
    Color color;
    if (sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      color = Color(0xFF0080).Dim();
    }
    else
    {
      color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0xFF0080);
    }
    MatrixOS::LED::SetColor(point, color);
  }

  // Copy
  {
    Point point = origin + Point(5, 0);
    Color color;
    if (sequencer->currentView == Sequencer::ViewMode::Mixer)
    {
      color = Color(0x0080FF).Dim();
    }
    else
    {
      color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : Color(0x0080FF);
    }
    MatrixOS::LED::SetColor(point, color);
  }

  if(sequencer->currentView == Sequencer::ViewMode::Sequencer)
  {
    // Octave -
    {
      Point point = origin + Point(6, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : GetOctaveMinusColor();
      MatrixOS::LED::SetColor(point, color);
    }

    // Octave +
    {
      Point point = origin + Point(7, 0);
      Color color = MatrixOS::KeyPad::GetKey(point)->Active() ? Color::White : GetOctavePlusColor();
      MatrixOS::LED::SetColor(point, color);
    }
  }
  else if(sequencer->currentView == Sequencer::ViewMode::Session)
  {
    Point point1 = origin + Point(6, 0);
    Point point2 = origin + Point(7, 0);
    MatrixOS::LED::SetColor(point1, Color::White.DimIfNot(sequencer->clipWindow == 0));
    MatrixOS::LED::SetColor(point2, Color::White.DimIfNot(sequencer->clipWindow == 1));
  }
  else // Go back to sequencer
  {
    Point point1 = origin + Point(6, 0);
    Point point2 = origin + Point(7, 0);
    bool active = MatrixOS::KeyPad::GetKey(point1)->Active() || MatrixOS::KeyPad::GetKey(point2)->Active();
    Color color = MatrixOS::KeyPad::GetKey(point1)->Active() ? Color::White : sequencer->meta.tracks[sequencer->track].color;
    MatrixOS::LED::SetColor(point1, color);
    MatrixOS::LED::SetColor(point2, color);
  }
  return true;
}