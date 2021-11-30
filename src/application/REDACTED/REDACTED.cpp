#include "REDACTED.h"

void REDACTED::Setup()
{
  x_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeX) - 8) / 2;
  y_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeY) - 8) / 2;

  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
  redactedTimer.RecordCurrent();
  redactedTimer2.RecordCurrent();
}

void REDACTED::Task1()
{
  if(redactedTimer.Tick(33 + (progress % 3 == 0), true))
  {
    progress ++;
    if(progress < 0)
      return;
    if(offset >= sizeof(data))
    {
      complete = true;
      return;
    }
    uint8_t bufferOffset = 1;
    for(uint8_t x = 0; x < 8; x ++)
    {
      if((data[offset] >> x) & 0x01)
      {
        for(uint8_t y = 0; y < 8; y++)
        {
          MatrixOS::LED::SetColor(Point(x + x_offset, y + y_offset), Color(((data[offset + bufferOffset] >> y) & 0x01) * 0xFFFFFF));
        }
        bufferOffset ++;
      }
    }
    MatrixOS::LED::Update(); 
    offset += bufferOffset;
  }
}

void REDACTED::Task2()
{
  if(redactedTimer2.Tick(54 + (progress2 % 3 == 0), true))
  {
    progress2++;
    waited++;
    if(waited >= wait)
    {
      waited = 0;
      wait = (data2[offset2] & 0x1F) + 1;
      uint8_t bufferOffset = 1;
      for(uint8_t i = 0; i <= (data2[offset2] >> 5); i++)
      {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, 0, data2[offset2 + bufferOffset] & 0x7F, 80 * (data2[offset2 + bufferOffset] >> 7)));

        bufferOffset ++;
      }
      offset2 += bufferOffset;
      if(offset2 >= sizeof(data2))
      {
          complete2 = true;
      }
    }
  }
}

void REDACTED::Loop()
{ 
  if(!complete) Task1();
  if(!complete2) Task2();
  if(complete && complete2) Exit();
}

void REDACTED::End()
{
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
}

void REDACTED::KeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        Exit();
        return;
    }
}