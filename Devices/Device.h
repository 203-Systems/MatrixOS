#pragma once

#include "Framework.h"
#include "MatrixOSConfig.h"
#include "Family.h"
#include "DefaultConfigs.h"

namespace Device
{
  /*
  Required Variables:
  const string name;
  const uint16_t led_count;
  const uint8_t x_size;
  const uint8_t y_size;
  string serial_number;
  */

  extern string name;
  extern uint8_t x_size;
  extern uint8_t y_size;
  extern string serial_number;


  void DeviceInit();
  void DeviceStart();
  void Reboot();
  void Bootloader();
  void ErrorHandler();

  uint64_t Micros();

  void DeviceSettings();

  void Log(string &format, va_list &valst);

  string GetSerial();

  namespace LED
  {
    extern const uint16_t fps;
    extern uint16_t count;
    extern uint8_t brightness_level[];
    extern uint8_t brightness_fine_level[];

    extern vector<LEDPartition> partitions;

    void Update(Color* frameBuffer, vector<uint8_t>& brightness);  // Render LED
    uint16_t XY2Index(Point xy);        // Grid XY to global buffer index, return UINT16_MAX if not index for given XY
    uint16_t ID2Index(uint16_t ledID);  // Local led Index to buffer index, return UINT16_MAX if not index for given
                                        // Index
    Point Index2XY(uint16_t index);     // Buffer index to Grid XY, return Point(INT16_MIN, INT16_MIN) if no XY found for
                                        // given index
                                        // Provides a way for application to iterate through all LEDs
  }

  namespace KeyPad
  {
    KeyInfo* GetKey(uint16_t keyID);
    void Clear();  // Since only the Device layer awares the keyInfo buffer, the function's job is to run Clear() on all
                   // keyInfo
    uint16_t XY2ID(Point xy);  // Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no
                               // ID is assigned to given XY
    Point ID2XY(uint16_t keyID);  // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                                  // given ID;
  }

  namespace NVS //Device should also implements duplication check. If new value is equal to the old one, then skip the write. 
  {
    size_t Size(uint32_t hash);
    vector<char> Read(uint32_t hash);
    bool Write(uint32_t hash, void* pointer, uint16_t length);
    bool Delete(uint32_t hash);
    void Clear();
  }

#if DEVICE_STORAGE == 1
  namespace Storage
  {
    // Storage subsystem is initialized automatically
    struct StorageStatus
    {
      bool available;          // Storage is available for read/write operations
      bool write_protected;    // Card is write protected
      uint32_t sector_count;   // Total number of sectors (0 if unavailable)
      uint16_t sector_size;    // Size of each sector in bytes (typically 512)
      uint32_t block_size;     // Erase block size in sectors (for filesystem optimization)
    };

    bool Available();
    const StorageStatus* Status();
    bool ReadSectors(uint32_t lba, uint32_t sector_count, void* dest);
    bool WriteSectors(uint32_t lba, uint32_t sector_count, const void* src);
  }
#endif

#if DEVICE_BATTERY == 1
  namespace Battery
  {
    bool Charging();
    float Voltage();
  }
#endif
}

// Matrix OS APIs available for Device Layer
namespace MatrixOS
{
  namespace Logging
  {
    void LogError(const string &tag, const string &format, ...);
    void LogWarning(const string &tag, const string &format, ...);
    void LogInfo(const string &tag, const string &format, ...);
    void LogDebug(const string &tag, const string &format, ...);
    void LogVerbose(const string &tag, const string &format, ...);
  }

  namespace SYS
  {
    void ErrorHandler(string error);
  }

  namespace KeyPad
  {
    bool NewEvent(KeyEvent* keyevent);
  }
}