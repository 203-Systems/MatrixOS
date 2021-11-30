#include "REDACTED.h"

void REDACTED::Setup()
{
  x_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeX) - 8) / 2;
  y_offset = (MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::MatrixSizeY) - 8) / 2;

  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
}

void REDACTED::Loop()
{ 
  if(redactedTimer.Tick(33 + (progress % 3 == 0)))
  {
    if(offset > sizeof(data))
      Exit();
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
    progress ++;
  }
}

void REDACTED::End()
{
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
  while(true){}
}

void REDACTED::KeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        Exit();
        return;
    }
}