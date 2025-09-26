#include "NoteControlBar.h"

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
    // Pitch Down
    if(xy == Point(0, CTL_BAR_Y - 1)) {
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

Color NoteControlBar::GetOctavePlusColor()
{
    return notePad[0]->rt->config->color;
}

Color NoteControlBar::GetOctaveMinusColor()
{
    return notePad[0]->rt->config->color;
}


bool NoteControlBar::Render(Point origin) {
    MatrixOS::LED::SetColor(origin + Point(0, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(0, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0xFF0000));
    MatrixOS::LED::SetColor(origin + Point(1, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(1, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : Color(0x00FF00));
    Color latchColor;
    if (notePad[0]->rt->noteLatch.IsToggleMode()) {
        latchColor = Color(0xFF00FF); // Magenta for toggle mode
    } else {
        latchColor = Color(0xFFFF00).DimIfNot(notePad[0]->rt->noteLatch.IsEnabled()); // Yellow for regular mode
    }
    MatrixOS::LED::SetColor(origin + Point(2, CTL_BAR_Y - 1), latchColor);
    MatrixOS::LED::SetColor(origin + Point(3, CTL_BAR_Y - 1), Color(0x404040));
    MatrixOS::LED::SetColor(origin + Point(4, CTL_BAR_Y - 1), Color(0x404040));
    MatrixOS::LED::SetColor(origin + Point(5, CTL_BAR_Y - 1), Color(0x404040));
    MatrixOS::LED::SetColor(origin + Point(6, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(6, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : GetOctaveMinusColor());
    MatrixOS::LED::SetColor(origin + Point(7, CTL_BAR_Y - 1), MatrixOS::KeyPad::GetKey(origin + Point(7, CTL_BAR_Y - 1))->Active() ? Color(0xFFFFFF) : GetOctavePlusColor());
    return true;
}