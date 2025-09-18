#include "REDACTED.h"

void REDACTED::Setup(const vector<string>& args) {
  // x_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeX) - 8) / 2; //TODO, wait for new API to
  // be implemented y_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeY) - 8) / 2;

  x_offset = 0;
  y_offset = 0;

  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();

  // MatrixOS::SYS::DelayMs(2000);
  redactedTimer.RecordCurrent();
  redactedTimer2.RecordCurrent();
}

void REDACTED::Task1() {
  if (redactedTimer.Tick(33 + (progress % 3 == 0), true))
  {
    progress++;
    if (progress < 0)
      return;
    if (offset >= sizeof(data))
    {
      complete = true;
      return;
    }
    uint8_t bufferOffset = 1;
    for (uint8_t x = 0; x < 8; x++)
    {
      if ((data[offset] >> x) & 0x01)
      {
        for (uint8_t y = 0; y < 8; y++)
        {
          MatrixOS::LED::SetColor(Point(x + x_offset, y + y_offset),
                                  Color(((data[offset + bufferOffset] >> y) & 0x01) * 0xFFFFFF));
        }
        bufferOffset++;
      }
    }
    // for(uint8_t i = 0; i < 64; i++)
    // {
    //   MatrixOS::LED::SetColor((1 << 12) + i, Color((i * 4)));
    // }
    MatrixOS::LED::Update();
    offset += bufferOffset;
  }
}

void REDACTED::Task2() {
  if (redactedTimer2.Tick(54 + (progress2 % 3 == 0), true))
  {
    progress2++;
    waited++;
    if (waited >= wait)
    {
      waited = 0;
      wait = (data2[offset2] & 0x1F) + 1;
      uint8_t bufferOffset = 1;
      for (uint8_t i = 0; i <= (data2[offset2] >> 5); i++)
      {
        uint8_t v = 80 * (data2[offset2 + bufferOffset] >> 7);
        MatrixOS::MIDI::Send(MidiPacket( v > 0 ? EMidiStatus::NoteOn : EMidiStatus::NoteOff, 0, data2[offset2 + bufferOffset] & 0x7F, v));

        bufferOffset++;
      }
      offset2 += bufferOffset;
      if (offset2 >= sizeof(data2))
      { complete2 = true; }
    }
  }
}

void REDACTED::Loop() {
  if (!complete)
    Task1();
  if (!complete2)
    Task2();
  if (complete && complete2)
    Exit();

  struct KeyEvent keyEvent;
  while (MatrixOS::KeyPad::Get(&keyEvent))
  { KeyEventHandler(&keyEvent); }
}

void REDACTED::End() {
  // for(uint8_t n = 0; n < 127; n ++)
  // {
  //   MatrixOS::MIDI::Send(MidiPacket::(NoteOff, n, 0));
  // }
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
}

void REDACTED::KeyEventHandler(KeyEvent* keyEvent) {
  if (keyEvent->id == 0 && keyEvent->info.state == PRESSED)
  {
    Exit();
    return;
  }
}