#include "NoteControlBar.h"
#include "ChordEffect.h"

NoteControlBar::NoteControlBar(Note* notePtr, NotePad* notepad1, NotePad* notepad2, UnderglowLight* underglow1, UnderglowLight* underglow2) {
    this->note = notePtr;
    this->notePad[0] = notepad1;
    this->notePad[1] = notepad2;
    this->underglow[0] = underglow1;
    this->underglow[1] = underglow2;
    this->shift[0] = 0;
    this->shift[1] = 0;
    this->shift_event[0] = false;
    this->shift_event[1] = false;
}

void NoteControlBar::SwapActiveConfig() {
    NotePadRuntime* padData1 = notePad[0]->rt;
    NotePadRuntime* padData2 = notePad[1]->rt;

    if(notePad[0]) {
        notePad[0]->SetPadRuntime(padData2);
    }
    if(notePad[1]) {
        notePad[1]->SetPadRuntime(padData1);
    }
    if(underglow[0]) {
        underglow[0]->SetColor(padData1->config->color);
    }
    if(underglow[1]) {
        underglow[1]->SetColor(padData1->config->color);
    }
}

bool NoteControlBar::ShiftActive() {
    return shift[0] != 0 || shift[1] != 0;
}

void NoteControlBar::ShiftEventOccured() {
    if(shift[0] != 0) {
        shift_event[0] = true;
    }
    else if(shift[1] != 0) {
        shift_event[1] = true;
    }
}

bool NoteControlBar::KeyEvent(Point xy, KeyInfo* keyInfo) {
    static uint32_t pitch_down = 0;
    static uint32_t pitch_up = 0;

    if(xy.y < CTL_BAR_Y - 1)
    {
        switch(mode) {
            case OFF_MODE:
                return false;
            case CHORD_MODE:
                return ChordControlKeyEvent(xy, keyInfo);
            case ARP_MODE:
                return ArpControlKeyEvent(xy, keyInfo);
            case KEY_MODE:
                return KeyControlKeyEvent(xy, keyInfo);
        }
    }

    // Control Bar
    // Pitch Down
    else if(xy == Point(0, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(ShiftActive()) {
                MatrixOS::MIDI::Send(MidiPacket::Stop());
            }
            else {
                pitch_down = MatrixOS::SYS::Millis();
            }
        }
        else if(keyInfo->State() == AFTERTOUCH)
        {
            if(pitch_down != 0 && pitch_down > pitch_up)
            {
                int32_t pitch_val = 8192 - (((uint16_t)keyInfo->Force() * 8192) >> 16);
                if(pitch_val < 0) {pitch_val = 0;}
                MLOGD("Note", "Pitch Bend: %d", pitch_val);
                MatrixOS::MIDI::Send(MidiPacket::PitchBend(notePad[0]->rt->config->channel, pitch_val));
            }
        }
        else if(keyInfo->State() == RELEASED) {
            if(pitch_down != 0 && pitch_down > pitch_up)
            {
                MatrixOS::MIDI::Send(MidiPacket::PitchBend(notePad[0]->rt->config->channel, 8192));
            }
            pitch_down = 0;
        }
        return true;
    }

    else if(xy == Point(1, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(ShiftActive()) {
                MatrixOS::MIDI::Send(MidiPacket::Start());
            }
            else {
                pitch_up = MatrixOS::SYS::Millis();
            }
        }
        else if(keyInfo->State() == AFTERTOUCH)
        {
            if(pitch_up != 0 && pitch_up >= pitch_down)
            {
                int32_t pitch_val = 8192 + (((uint16_t)keyInfo->Force() * 8191) >> 16);
                if(pitch_val > 16383) {pitch_val = 16383;}
                MLOGD("Note", "Pitch Bend: %d", pitch_val);
                MatrixOS::MIDI::Send(MidiPacket::PitchBend(notePad[0]->rt->config->channel, pitch_val));
            }
        }
        else if(keyInfo->State() == RELEASED) {
            if(pitch_up != 0 && pitch_up >= pitch_down)
            {
                MatrixOS::MIDI::Send(MidiPacket::PitchBend(notePad[0]->rt->config->channel, 8192));
            }
            pitch_up = 0;
        }
        return true;
    }

    else if(xy == Point(2, CTL_BAR_Y - 1)) {
       if(keyInfo->State() == PRESSED) {
            if(ShiftActive()) {
                notePad[0]->rt->noteLatch.SetToggleMode(!notePad[0]->rt->noteLatch.IsToggleMode());
            }
            else
            {
                notePad[0]->rt->noteLatch.SetEnabled(!notePad[0]->rt->noteLatch.IsEnabled());
            }
       }
       return true;
    }

    // Chord Mode
    else if(xy == Point(3, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(mode == CHORD_MODE) {
                mode = OFF_MODE;
                notePad[0]->rt->chordEffect.SetEnabled(false);
            } else {
                mode = CHORD_MODE;
                notePad[0]->rt->chordEffect.SetEnabled(true);
            }
        }
        return true;
    }

    // Key Mode
    else if(xy == Point(5, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(mode == KEY_MODE)
            {
                mode = OFF_MODE;
            }
            else
            {
                mode = KEY_MODE;
            }
        }
        return true;
    }

    // Octave Down
    else if(xy == Point(6, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(!ShiftActive()) {
                shift[0] = MatrixOS::SYS::Millis();
            }
            else {
                SwapActiveConfig();
                ShiftEventOccured();
            }
        }
        else if(keyInfo->State() == RELEASED) {
            if((MatrixOS::SYS::Millis() - shift[0] < hold_threshold) && shift_event[0] == false) {
                int8_t new_octave = notePad[0]->rt->config->octave - 1;
                notePad[0]->rt->config->octave = new_octave < 0 ? 0 : new_octave;
                notePad[0]->GenerateKeymap();
            }
            shift[0] = 0;
            shift_event[0] = false;
        }
        return true;
    }

    // Octave Up
    else if(xy == Point(7, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(!ShiftActive()) {
                shift[1] = MatrixOS::SYS::Millis();
            }
            else {
                SwapActiveConfig();
                ShiftEventOccured();
            }
        }
        else if(keyInfo->State() == RELEASED) {
            if((MatrixOS::SYS::Millis() - shift[1] < hold_threshold) && shift_event[1] == false) {
                int8_t new_octave = notePad[0]->rt->config->octave + 1;
                notePad[0]->rt->config->octave = new_octave > 7 ? 7 : new_octave; 
                notePad[0]->GenerateKeymap();
            }
            shift[1] = 0;
            shift_event[1] = false;
        }
        return true;
    }
    return false;
}

bool NoteControlBar::ChordControlKeyEvent(Point xy, KeyInfo* keyInfo) {
    // Chord buttons are on the row CTL_BAR_Y - 2 (row 2)
    // x0-x7: Dim, min, maj, sus, ext6, extMin7, extMaj7, ext9
    if (xy.y == CTL_BAR_Y - 2) {
        ChordCombo& combo = notePad[0]->rt->chordEffect.GetChordCombo();

        if (keyInfo->state == PRESSED) {
            // Toggle the chord bit based on x position
            switch(xy.x) {
                case 0: combo.dim = !combo.dim; break;
                case 1: combo.min = !combo.min; break;
                case 2: combo.maj = !combo.maj; break;
                case 3: combo.sus = !combo.sus; break;
                case 4: combo.ext6 = !combo.ext6; break;
                case 5: combo.extMin7 = !combo.extMin7; break;
                case 6: combo.extMaj7 = !combo.extMaj7; break;
                case 7: combo.ext9 = !combo.ext9; break;
            }

            // Update the chord effect with new combo
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        }
        else if (keyInfo->state == RELEASED && !ShiftActive()) {
            // On release without shift, clear the selection
            switch(xy.x) {
                case 0: combo.dim = false; break;
                case 1: combo.min = false; break;
                case 2: combo.maj = false; break;
                case 3: combo.sus = false; break;
                case 4: combo.ext6 = false; break;
                case 5: combo.extMin7 = false; break;
                case 6: combo.extMaj7 = false; break;
                case 7: combo.ext9 = false; break;
            }

            // Update the chord effect with cleared combo
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        }
        return true;
    }
    return false;
}
bool NoteControlBar::ArpControlKeyEvent(Point xy, KeyInfo* keyInfo) {
    return false;
}

bool NoteControlBar::KeyControlKeyEvent(Point xy, KeyInfo* keyInfo) {
    if(xy.y < CTL_BAR_Y - 3)
    {
        return false;
    }

    xy = xy - Point(0, CTL_BAR_Y - 3);

    if (xy.x == 7 || xy == Point(0, 0) || xy == Point(3, 0))
    {
      return true;
    }
    notePad[0]->rt->config->rootKey = xy.x * 2 + xy.y - 1 - (xy.x > 2);
    return true;
}

Color NoteControlBar::GetOctavePlusColor() {
    return notePad[0]->rt->config->color;
}

Color NoteControlBar::GetOctaveMinusColor() {
    return notePad[0]->rt->config->color;
}


bool NoteControlBar::Render(Point origin) {
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(0, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0xFF0000));
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(1, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0x00FF00));
    Color latchColor;
    if (notePad[0]->rt->noteLatch.IsEnabled()) {
        latchColor = Color(0xFFFFFF); // White when enabled
    } else if(notePad[0]->rt->noteLatch.IsToggleMode()) {
        latchColor = Color(0xFF00FF);
    } else {
        latchColor = Color(0xFFFF00);
    }
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 1), latchColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 1), mode == CHORD_MODE ? Color(0xFFFFFF) : Color(0x00FFFF));
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 1), Color(0x404040));
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 1), mode == KEY_MODE ? Color(0xFFFFFF) : Color(0xFF0090));
    MatrixOS::LED::SetColor(origin + Point(6, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(6, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : GetOctaveMinusColor());
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(7, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : GetOctavePlusColor());

    switch(mode) {
        case OFF_MODE:
        break;
        case CHORD_MODE:
            RenderChordControl(origin);
        break;
        case ARP_MODE:
            RenderArpControl(origin);
        break;
        case KEY_MODE:
            RenderKeyControl(origin);
        break;

    }
    return true;
}

void NoteControlBar::RenderChordControl(Point origin) {
    // Get current chord combo from the effect
    ChordCombo& combo = notePad[0]->rt->chordEffect.GetChordCombo();

    // Colors for different chord types
    Color baseColor = Color(0x0080FF);
    Color extColor = Color(0x00FFFF);

    // Row CTL_BAR_Y - 2: Chord base and extensions
    // x0: Dim, x1: min, x2: maj, x3: sus
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 2), combo.dim ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 2), combo.min ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 2), combo.maj ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 2), combo.sus ? Color(0xFFFFFF) : baseColor);

    // x4: ext6, x5: extMin7, x6: extMaj7, x7: ext9
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 2), combo.ext6 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 2), combo.extMin7 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(6, CTL_BAR_Y - 2), combo.extMaj7 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 2), combo.ext9 ? Color(0xFFFFFF) : extColor);
}

void NoteControlBar::RenderArpControl(Point origin) {

}

void NoteControlBar::RenderKeyControl(Point origin) {
    uint16_t c_aligned_scale_map =
        ((notePad[0]->rt->config->scale << notePad[0]->rt->config->rootKey) + ((notePad[0]->rt->config->scale & 0xFFF) >> (12 - notePad[0]->rt->config->rootKey % 12))) & 0xFFF;  // Root key should always < 12,
                                                                                      // might add an assert later
    for (uint8_t note = 0; note < 12; note++)
    {
      Point xy = origin + Point(CTL_BAR_Y - 3) + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
      if (note == notePad[0]->rt->config->rootKey)
      { MatrixOS::LED::SetColor(xy, notePad[0]->rt->config->rootColor); }
      else if (bitRead(c_aligned_scale_map, note))
      { MatrixOS::LED::SetColor(xy, notePad[0]->rt->config->color); }
      else
      { MatrixOS::LED::SetColor(xy, notePad[0]->rt->config->color.DimIfNot()); }
    }
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 3), Color(0));
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 3), Color(0));
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 3), Color(0));
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 2), Color(0));
}
