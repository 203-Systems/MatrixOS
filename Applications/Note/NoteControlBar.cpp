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
    note->activeConfig = note->activeConfig.Get() == 0 ? 1 : 0;
    if(underglow[0]) {
        underglow[0]->SetColor(notePad[0]->rt->config->color);
    }
    if(underglow[1]) {
        underglow[1]->SetColor(notePad[1]->rt->config->color);
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

void NoteControlBar::ShiftClear() {
    shift[0] = 0;
    shift[1] = 0;
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

    // Arp Mode
    else if(xy == Point(4, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(ShiftActive()) {
                ShiftClear();
                note->ArpConfigMenu();
            } else if(mode == ARP_MODE) {
                mode = OFF_MODE;
            } else {
                mode = ARP_MODE;
            }
        }
        return true;
    }

    // Key Mode
    else if(xy == Point(5, CTL_BAR_Y - 1)) {
        if(keyInfo->State() == PRESSED) {
            if(ShiftActive()) {
                ShiftClear();
                note->ScaleSelector();
                notePad[0]->GenerateKeymap();
            } else if(mode == KEY_MODE) {
                mode = OFF_MODE;
            } else {
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
                notePad[0]->rt->config->octave = new_octave < -2 ? -2 : new_octave;
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
                notePad[0]->rt->config->octave = new_octave > 12 ? 12 : new_octave; 
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
    if(xy.y < CTL_BAR_Y - 3 || xy.y > CTL_BAR_Y - 2) {
        return false;
    }


    // Section 1: Top left 4 buttons (CTL_BAR_Y - 3, x0-x3) - Basic chord types
    if (xy.y == CTL_BAR_Y - 3 && xy.x <= 3) {
        ChordCombo& combo = notePad[0]->rt->chordEffect.chordCombo;

        if (keyInfo->State() == PRESSED) {
            // Clear all basic chord types first (mutually exclusive)
            combo.dim = false;
            combo.min = false;
            combo.maj = false;
            combo.sus = false;

            // Set only the selected one
            switch(xy.x) {
                case 0: combo.dim = true; break;
                case 1: combo.min = true; break;
                case 2: combo.maj = true; break;
                case 3: combo.sus = true; break;
            }
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        } else if (keyInfo->State() == RELEASED && !ShiftActive()) {
            switch(xy.x) {
                case 0: combo.dim = false; break;
                case 1: combo.min = false; break;
                case 2: combo.maj = false; break;
                case 3: combo.sus = false; break;
            }
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        }

        return true;
    }

    // Section 2: Bottom left 4 buttons (CTL_BAR_Y - 2, x0-x3) - Extensions
    if (xy.y == CTL_BAR_Y - 2 && xy.x <= 3) {
        ChordCombo& combo = notePad[0]->rt->chordEffect.chordCombo;

        if (keyInfo->State() == PRESSED) {
            switch(xy.x) {
                case 0: combo.ext6 = !combo.ext6; break;
                case 1: combo.extMin7 = !combo.extMin7; break;
                case 2: combo.extMaj7 = !combo.extMaj7; break;
                case 3: combo.ext9 = !combo.ext9; break;
            }
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        } else if (keyInfo->State() == RELEASED && !ShiftActive()) {
            switch(xy.x) {
                case 0: combo.ext6 = false; break;
                case 1: combo.extMin7 = false; break;
                case 2: combo.extMaj7 = false; break;
                case 3: combo.ext9 = false; break;
            }
            notePad[0]->rt->chordEffect.SetChordCombo(combo);
        }
        return true;
    }

    // Section 3: Right 8 buttons (x4-x7 on both rows) - Inversion controls (0-7)
    if (xy.x >= 4 && xy.x <= 7 && keyInfo->State() == PRESSED) {
        uint8_t inversion;
        if (xy.y == CTL_BAR_Y - 3) {
            // Top row: inversion 0-3
            inversion = xy.x - 4;
        } else if (xy.y == CTL_BAR_Y - 2) {
            // Bottom row: inversion 4-7
            inversion = xy.x;
        } else {
            return false;
        }

        notePad[0]->rt->chordEffect.SetInversion(inversion);
        return true;
    }

    return false;
}
bool NoteControlBar::ArpControlKeyEvent(Point xy, KeyInfo* keyInfo) {
    if(xy.y != CTL_BAR_Y - 2 || keyInfo->State() != PRESSED) {
        return false;
    }

    // Map slider positions to ArpDivision values
    ArpDivision divisions[8] = {
            DIV_OFF, // 0
            // DIV_WHOLE, // 1
            // DIV_HALF, // 2
            // DIV_THIRD, // 3
            DIV_QUARTER, // 4
            DIV_SIXTH, // 6
            DIV_EIGHTH, // 8
            DIV_TWELFTH, // 12
            DIV_SIXTEENTH, // 16
            DIV_TWENTYFOURTH, // 24
            DIV_THIRTYSECOND, // 32
            // DIV_SIXTYFOURTH, // 64
    };

    if(xy.x >= 0 && xy.x < 8) {
        notePad[0]->rt->arpeggiator.SetDivision(divisions[xy.x]);

        return true;
    }

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
    notePad[0]->GenerateKeymap();
    return true;
}

const uint8_t OctaveGradient[8]  = {0, 16, 42, 68, 124, 182, 255};

Color NoteControlBar::GetOctavePlusColor() {
    int8_t octave = notePad[0]->rt->config->octave;
    uint8_t brightness;

    if (octave >= 4) {
        brightness = 255;
    } else {
        // Use gradient for octaves below 4 - dimmer as it goes down
        uint8_t index = (4 - octave) > 6 ? 6 : (4 - octave);
        brightness = OctaveGradient[6 - index];
    }

    return notePad[0]->rt->config->color.Scale(brightness);
}

Color NoteControlBar::GetOctaveMinusColor() {
    int8_t octave = notePad[0]->rt->config->octave;
    uint8_t brightness;

    if (octave <= 4) {
        brightness = 255;
    } else {
        // Use gradient for octaves above 4 - dimmer as it goes up
        uint8_t index = (octave - 4) > 6 ? 6 : (octave - 4);
        brightness = OctaveGradient[6 - index];
    }

    return notePad[0]->rt->config->color.Scale(brightness);
}


bool NoteControlBar::Render(Point origin) {
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(0, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0xFF0000));
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(1, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0x00FF00));
    Color latchColor;
    if (notePad[0]->rt->noteLatch.IsToggleMode()) {
        latchColor = Color(0xFF00FF);
    } else if(notePad[0]->rt->noteLatch.IsEnabled()) {
        latchColor = Color(0xFFFFFF); // White when enabled
    } else {
        latchColor = Color(0xFFFF00); 
    }
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 1), latchColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 1), mode == CHORD_MODE ? Color(0xFFFFFF) : Color(0x00FFFF));
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 1), mode == ARP_MODE ? Color(0xFFFFFF) : Color(0x80FF00));
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 1), mode == KEY_MODE ? Color(0xFFFFFF) : Color(0xFF0060));
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
    ChordCombo& combo = notePad[0]->rt->chordEffect.chordCombo;

    // Colors for different chord types
    Color baseColor = Color(0xFF00FF);
    Color extColor = Color(0x00FFFF);
    Color inversionColor = Color(0xFFFF00);

    // Row 0
    // x0: Dim, x1: min, x2: maj, x3: sus
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 3), combo.dim ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 3), combo.min ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 3), combo.maj ? Color(0xFFFFFF) : baseColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 3), combo.sus ? Color(0xFFFFFF) : baseColor);

    // Row 2
    // x0: ext6, x1: extMin7, x2: extMaj7, x3: ext9
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 2), combo.ext6 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 2), combo.extMin7 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 2), combo.extMaj7 ? Color(0xFFFFFF) : extColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 2), combo.ext9 ? Color(0xFFFFFF) : extColor);

    // Right side
    // Inversion
    int8_t currentInversion = notePad[0]->rt->chordEffect.inversion;
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 3), inversionColor.DimIfNot(currentInversion >= 0));
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 3), inversionColor.DimIfNot(currentInversion >= 1));
    MatrixOS::LED::SetColor(origin + Point(6, CTL_BAR_Y - 3), inversionColor.DimIfNot(currentInversion >= 2));
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 3), inversionColor.DimIfNot(currentInversion >= 3));
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 2), inversionColor.DimIfNot(currentInversion >= 4));
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 2), inversionColor.DimIfNot(currentInversion >= 5));
    MatrixOS::LED::SetColor(origin + Point(6, CTL_BAR_Y - 2), inversionColor.DimIfNot(currentInversion >= 6));
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 2), inversionColor.DimIfNot(currentInversion >= 7));
}

void NoteControlBar::RenderArpControl(Point origin) {
    // Map ArpDivision enum to slider positions (0-7)
    ArpDivision divisions[8] = {
            DIV_OFF, // 0
            // DIV_WHOLE, // 1
            // DIV_HALF, // 2
            // DIV_THIRD, // 3
            DIV_QUARTER, // 4
            DIV_SIXTH, // 6
            DIV_EIGHTH, // 8
            DIV_TWELFTH, // 12
            DIV_SIXTEENTH, // 16
            DIV_TWENTYFOURTH, // 24
            DIV_THIRTYSECOND, // 32
            // DIV_SIXTYFOURTH, // 64
    };

    // Render slider
    for (uint8_t x = 0; x < 8; x++) {
        Color color;
        if (notePad[0]->rt->arpeggiator.division >= divisions[x]) {
            color = Color(0x80FF00);
        } else {
            color = Color(0x80FF00).Dim();
        }
        MatrixOS::LED::SetColor(origin + Point(x, CTL_BAR_Y - 2), color);
    }
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
