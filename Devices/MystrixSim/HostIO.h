#pragma once

#include "MatrixOS.h"

namespace MystrixSim::HostIO
{
enum class PythonSessionMode : uint8_t {
  None = 0,
  Repl = 1,
  App = 2,
};

bool UsbConnected();
void SetUsbConnected(bool connected);

void QueueSerialInput(const uint8_t* data, size_t size);
void ClearSerialInput();
uint32_t SerialInputAvailable();
int ReadSerialInputByte();
uint32_t ReadSerialInput(void* buffer, uint32_t length);
string ReadSerialInputString();

void QueuePythonInput(const uint8_t* data, size_t size);
void ClearPythonInput();
uint32_t PythonInputAvailable();
int ReadPythonInputByte();
void SetPythonSessionMode(PythonSessionMode mode);
PythonSessionMode GetPythonSessionMode();
void TapPythonOutput(PythonSessionMode mode, const string& text);

bool StagePythonScript(const string& fileName, const string& contents);
bool GetPythonScript(const string& path, string* contents);
string GetStagedPythonScriptPath();
void ClearStagedPythonScript();

void TapSerial(int direction, const string& text);
void TapMidi(int direction, uint16_t srcPort, uint16_t dstPort, const MidiPacket& midiPacket);

bool NewRawHidReport(const uint8_t* report, size_t size);
} // namespace MystrixSim::HostIO
