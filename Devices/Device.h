#pragma once

#include "Framework.h"
#include "KeyScanState.h"
#include "MatrixOSConfig.h"
#include "Family.h"
#include "DefaultConfigs.h"

namespace Device
{
/*
Required Variables:
const string name;
const uint16_t led_count;
const uint8_t xSize;
const uint8_t ySize;
string serialNumber;
*/

extern string name;
extern uint8_t xSize;
extern uint8_t ySize;
extern string serialNumber;

void DeviceInit();
void DeviceStart();
void Reboot();
void Bootloader();
void ErrorHandler();

uint64_t Micros();

void DeviceSettings();

InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
void Rotate(Direction rotation, bool absolute = false);

void Log(string& format, va_list& valst);

string GetSerial();

namespace LED
{
extern const uint16_t fps;
extern uint16_t count;
extern uint8_t brightnessLevel[];
extern uint8_t brightnessFineLevel[];

extern vector<LEDPartition> partitions;

void Update(Color* frameBuffer, vector<uint8_t>& brightness); // Render LED
uint16_t XY2Index(Point xy);                                  // Grid XY to global buffer index, return UINT16_MAX if not index for given XY
uint16_t ID2Index(uint16_t ledID);                            // Local led Index to buffer index, return UINT16_MAX if not index for given
                                                              // Index
Point Index2XY(uint16_t index); // Buffer index to Grid XY, return Point(INT16_MIN, INT16_MIN) if no XY found for
                                // given index
                                // Provides a way for application to iterate through all LEDs
} // namespace LED

namespace KeyPad
{
KeyScanState* GetKey(uint16_t keyID);
void Clear();                // Clears all key scan state buffers (fnState, keypadState, touchbarState)
uint16_t XY2ID(Point xy);    // Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no
                             // ID is assigned to given XY
Point ID2XY(uint16_t keyID); // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                             // given ID;

// Legacy bridge: device-owned decoding of old keyID encoding into InputId.
// Moves topology knowledge (cluster IDs, memberId formulas) to the device layer.
InputId BridgeKeyId(uint16_t keyID);
uint16_t InputIdToLegacyKeyId(InputId id);
} // namespace KeyPad

namespace Input
{
// Device-owned cluster data. The device layer populates and maintains this.
extern vector<InputCluster> clusters;

// Device-owned coordinate mapping handlers.
// The device layer defines how memberId maps to visual coordinates and vice versa.
bool TryGetPoint(uint8_t clusterId, uint16_t memberId, Point* point);
bool TryGetMemberId(uint8_t clusterId, Point point, uint16_t* memberId);
} // namespace Input

namespace NVS // Device should also implements duplication check. If new value is equal to the old one, then skip the write.
{
size_t Size(uint32_t hash);
vector<char> Read(uint32_t hash);
bool Write(uint32_t hash, void* pointer, uint16_t length);
bool Delete(uint32_t hash);
void Clear();
} // namespace NVS

#if DEVICE_STORAGE == 1
namespace Storage
{
// Storage subsystem is initialized automatically
struct StorageStatus {
  bool available;        // Storage is available for read/write operations
  bool writeProtected;  // Card is write protected
  uint32_t sectorCount; // Total number of sectors (0 if unavailable)
  uint16_t sectorSize;  // Size of each sector in bytes (typically 512)
  uint32_t blockSize;   // Erase block size in sectors (for filesystem optimization)
};

bool Available();
const StorageStatus* Status();
bool ReadSectors(uint32_t lba, uint32_t sectorCount, void* dest);
bool WriteSectors(uint32_t lba, uint32_t sectorCount, const void* src);
} // namespace Storage
#endif

#if DEVICE_BATTERY == 1
namespace Battery
{
bool Charging();
float Voltage();
} // namespace Battery
#endif
} // namespace Device

// Matrix OS APIs available for Device Layer
namespace MatrixOS
{
namespace Logging
{
void LogError(const string& tag, const string& format, ...);
void LogWarning(const string& tag, const string& format, ...);
void LogInfo(const string& tag, const string& format, ...);
void LogDebug(const string& tag, const string& format, ...);
void LogVerbose(const string& tag, const string& format, ...);
} // namespace Logging

namespace SYS
{
void ErrorHandler(string error);
}

namespace Input
{
bool NewEvent(const InputEvent& event);
}
} // namespace MatrixOS