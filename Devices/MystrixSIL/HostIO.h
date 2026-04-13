#pragma once

#include "MatrixOS.h"

namespace MystrixSIL::HostIO
{
bool UsbConnected();
void SetUsbConnected(bool connected);

void TapSerial(int direction, const string& text);
void TapMidi(int direction, uint16_t srcPort, uint16_t dstPort, const MidiPacket& midiPacket);

bool NewRawHidReport(const uint8_t* report, size_t size);
} // namespace MystrixSIL::HostIO