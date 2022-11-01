#pragma once

#include "Device.h"
#include "FreeRTOS.h"
#include "framework/Framework.h"
#include "queue.h"
#include "semphr.h"
#include "system/Parameters.h"
#include "system/SystemVariables.h"
#include "system/UserVariables.h"
#include "task.h"
#include "timers.h"
#include "tusb.h"
#include "usb/MidiSpecs.h"

#define noexpose  // Custum keyword to remove function to be generated as exposed API

class Application;
class Application_Info;

// Matrix OS Modules and their API for Application layer or system layer
namespace MatrixOS
{
  inline uint32_t api_version = 0;
  inline Application* active_app = NULL;

  namespace SYS
  {
    inline bool inited = false;
    void Init(void);

    uint32_t Millis(void);
    void DelayMs(uint32_t intervalMs);

    void Reboot(void);
    void Bootloader(void);

    void OpenSetting(void);

    void Rotate(EDirection rotation, bool absolute = false);
    void NextBrightness();

    void ExecuteAPP(string author, string app_name);
    void ExitAPP();

    void ErrorHandler(string error = NULL);
  }

  namespace LED
  {
    void Init(void);
    void SetColor(Point xy, Color color, uint8_t layer = 255);
    void SetColor(uint16_t ID, Color color, uint8_t layer = 255);
    void Fill(Color color, uint8_t layer = 255);
    void Update(uint8_t layer = 255);

    int8_t CurrentLayer();
    int8_t CreateLayer();
    bool DestoryLayer();

    void ShiftCanvas(EDirection direction, int8_t distance, uint8_t layer = 255);
    void RotateCanvas(EDirection direction, uint8_t layer = 255);

    void PauseUpdate(bool pause);
  }

  namespace KEYPAD
  {
    noexpose void Init(void);
    uint16_t Scan();                    // Return # of changed key
    bool NewEvent(KeyEvent* keyevent);  // Adding keyevent, return true when queue is full
    bool Get(KeyEvent* keyEvent_dest, uint32_t timeout_ms = 0);
    KeyInfo* GetKey(Point keyXY);
    KeyInfo* GetKey(uint16_t keyID);
    void Clear();              // Don't handle any keyEvent till their next Press event (So no Release, Hold, etc)
    void ClearList();          // Clear the current KeyEvent queue
    uint16_t XY2ID(Point xy);  // Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no
                               // ID is assigned to given XY
    Point ID2XY(uint16_t keyID);  // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                                  // given ID;

  }

  namespace USB
  {
    noexpose void Init();

    bool Inited(void);     // If USB Stack is initlized, not sure what it will be needed but I added it anyways
    bool Connected(void);  // If USB is connected

    namespace CDC
    {
      bool Connected(void);
      uint32_t Available(void);
      void Poll(void);

      void Print(string str);
      void Println(string str);
      void Printf(string format, ...);
      void VPrintf(string format, va_list valst);
      void Flush(void);

      int8_t Read(void);
      uint32_t ReadBytes(void* buffer, uint32_t length);  // Returns nums byte read
      string ReadString(void);
    }
  }

  namespace MIDI
  {
    noexpose void Init(void);

    bool Get(MidiPacket* midipacket_dest, uint16_t timeout_ms = 0);
    bool Send(MidiPacket midiPacket);

    // Those APIs are only for MidiPort to use
    noexpose bool RegisterMidiPort(uint16_t port_id, MidiPort* midiPort);
    noexpose void UnregisterMidiPort(uint16_t port_id);
    noexpose bool Recive(MidiPacket midipacket_prt, uint32_t timeout_ms = 0);
  }

  namespace Logging
  {
    void LogLevelSet(string tag, ELogLevel level);
    void LogError(string tag, string format, ...);
    void LogWarning(string tag, string format, ...);
    void LogInfo(string tag, string format, ...);
    void LogDebug(string tag, string format, ...);
    void LogVerbose(string tag, string format, ...);
  }

  namespace NVS
  {
    vector<char> GetVariable(uint32_t hash);
    int8_t GetVariable(uint32_t hash, void* pointer, uint16_t length);  // Load variable into pointer. If not defined,
                                                                        // it will try to assign current pointer value
                                                                        // into it.
    bool SetVariable(uint32_t hash, void* pointer, uint16_t length);
    bool DeleteVariable(uint32_t hash);
  }

  // namespace GPIO
  // {
  //   enum EMode {Input = 1, Output = 2, Pwm = 4, PullUp = 8, PullDown = 16};

  //   void DigitalWrite(EPin pin, bool value);
  //   bool DigitalRead(EPin pin);
  //   void AnalogWrite(EPin pin, int value);
  //   int AnalogRead(EPin pin);
  //   void PinMode(EPin pin, EMode mode);

  //   namespace I2C
  //   {
  //     bool BeginTransmission(uint8_t address);
  //     bool RequestFrom(uint8_t address, uint8_t bytes);
  //     bool Write(uint8_t data);
  //     uint8_t Read(void);
  //     bool EndTransmission(bool stop = true);
  //   }

  //   namespace UART
  //   {
  //     enum EConfig {length8 = 0, length9 = 0x10, stopBits1 = 0, stopBits15 = 0x1, stopBits2 = 0x2,
  //       parityNone = 0, parityEven = 0x4, parityOdd = 0x08, flowNone = 0, flowHw = 0x20};
  //     enum EConfigMask {length = EConfig::length8 | EConfig::length9, stopBits = EConfig::stopBits1 |
  //     EConfig::stopBits15 |
  //         EConfig::stopBits2, parity = EConfig::parityNone | EConfig::parityEven | EConfig::parityOdd, flow =
  //         EConfig::flowNone | EConfig::flowHw
  //     };
  //     void Setup(int baudrate, EConfig config);
  //     bool Available(void);
  //     uint8_t Read(void);
  //     void Write(uint8_t);
  //   }

  namespace UIInterface
  {
    void TextScroll(string ascii, Color color, uint16_t speed = 10, bool loop = false);
    int32_t NumberSelector8x8(int32_t value, Color color, string name, int32_t lower_limit, int32_t upper_limit);
  }
}