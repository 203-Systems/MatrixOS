#pragma once

namespace MatrixOS::HID
{
void Init(void);
void Reset(void);

namespace RawHID
{
void Init(void);
bool NewReport(const uint8_t* report, size_t size);
}
} // namespace MatrixOS::HID
