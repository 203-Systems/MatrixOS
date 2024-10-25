#pragma once

#include "Device.h"
#include "FreeRTOS.h"
#include "framework/Framework.h"
#include "queue.h"
#include "semphr.h"
#include "system/Parameters.h"
#include "system/UserVariables.h"
#include "task.h"
#include "timers.h"
#include "tusb.h"

#include "./system/HID/HIDSpecs.h"

#define noexpose  // Custom keyword to remove function to be generated as exposed API

class Application;
class Application_Info;

// Matrix OS Modules and their API for Application layer or system layer
namespace MatrixOS
{
  inline uint32_t api_version = 0;

  namespace SYS
  {
    inline bool inited = false;
    void Begin(void);
    void InitSysModules(void);

    uint32_t Millis(void);
    void DelayMs(uint32_t intervalMs);

    void Reboot(void);
    void Bootloader(void);

    void OpenSetting(void);

    void Rotate(EDirection rotation, bool absolute = false);

    void ExecuteAPP(string author, string app_name);
    void ExitAPP();

    void ErrorHandler(string error = string());
  }

  namespace LED
  {
    void Init(void);

    void NextBrightness();
    void SetBrightness(uint8_t brightness);
    void SetBrightnessMultiplier(string partition_name, float multiplier);

    void SetColor(Point xy, Color color, uint8_t layer = 255);
    void SetColor(uint16_t ID, Color color, uint8_t layer = 255);
    void Fill(Color color, uint8_t layer = 255);
    void FillPartition(string partition, Color color, uint8_t layer = 255);
    void Update(uint8_t layer = 255);

    int8_t CurrentLayer();
    int8_t CreateLayer(uint16_t crossfade = crossfade_duration);
    void CopyLayer(uint8_t dest, uint8_t src);
    bool DestroyLayer(uint16_t crossfade = crossfade_duration);

    void Fade(uint16_t crossfade = crossfade_duration, Color* source_buffer = nullptr);

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

    bool Inited(void);     // If USB Stack is initialized, not sure what it will be needed but I added it anyways
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

    bool Get(MidiPacket* midiPacketDest, uint16_t timeout_ms = 0);
    bool Send(MidiPacket midiPacket, uint16_t timeout_ms = 0);
    bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta = true);  // If include meta, it will send the correct header and ending;

    // Those APIs are only for MidiPort to use
    noexpose bool OpenMidiPort(uint16_t port_id, MidiPort* midiPort);
    noexpose void CloseMidiPort(uint16_t port_id);
    noexpose bool Receive(MidiPacket midipacket_prt, uint32_t timeout_ms = 0);
  }

  namespace HID
  {
    void Init();
    bool Ready(void);
    
    namespace Keyboard
    {
      bool Write(KeyboardKeycode k);
      bool Press(KeyboardKeycode k);
      bool Release(KeyboardKeycode k);
      bool Remove(KeyboardKeycode k);
      void ReleaseAll(void);
    }

    namespace Mouse
    {
      void Click(MouseKeycode b = MOUSE_LEFT);
      void press(MouseKeycode b = MOUSE_LEFT);   // press LEFT by default
      void release(MouseKeycode b = MOUSE_LEFT); // release LEFT by default
      void ReleaseAll(void);
      void Move(signed char x, signed char y, signed char wheel = 0);
    }

    namespace Touch // Absolute Mouse
    {
      void Click(MouseKeycode b = MOUSE_LEFT);
      void Press(MouseKeycode b = MOUSE_LEFT);   // press LEFT by default
      void Release(MouseKeycode b = MOUSE_LEFT); // release LEFT by default
      void ReleaseAll(void);
      void MoveTo(signed char x, signed char y, signed char wheel = 0);
      void Move(signed char x, signed char y, signed char wheel = 0);
    }

    namespace Gamepad
    {
      void Press(GamepadKeycode b);
      void Release(GamepadKeycode b);
      void ReleaseAll(void);

      void Buttons(uint32_t b);
      void XAxis(int8_t a);
      void YAxis(int8_t a);
      void ZAxis(int8_t a);
      void RXAxis(int8_t a);
      void RYAxis(int8_t a);
      void RZAxis(int8_t a);
      void DPad(GamepadDPadDirection d);
    }

    namespace Consumer
    {
      void Write(ConsumerKeycode c);
      void Press(ConsumerKeycode c);
      void Release(ConsumerKeycode c);
      void ReleaseAll(void);
    }

    namespace System
    {
      void Write(SystemKeycode s);
      void Press(SystemKeycode s);
      void Release(void);
      void ReleaseAll(void);
    }

    namespace RawHID
    {
      void Init();
      size_t Get(uint8_t** report, uint32_t timeout_ms = 0);
      bool Send(const vector<uint8_t> &report);
    }
  }

  namespace Logging
  {
    // Regular function version - not recommended
    void LogError(const string &tag, const string &format, ...);
    void LogWarning(const string &tag, const string &format, ...);
    void LogInfo(const string &tag, const string &format, ...);
    void LogDebug(const string &tag, const string &format, ...);
    void LogVerbose(const string &tag, const string &format, ...);

    // Macro version is perfered because it will not generate any code if the log level is lower than the log level
    #if (MATRIXOS_LOG_LEVEL >= LOG_LEVEL_ERROR)
      #define MLOGE(tag, format, ...) MatrixOS::Logging::LogError(tag, format, ##__VA_ARGS__)
    #else
      #define MLOGE(tag, format, ...)
    #endif
    
    #if (MATRIXOS_LOG_LEVEL >= LOG_LEVEL_WARNING)
      #define MLOGW(tag, format, ...) MatrixOS::Logging::LogWarning(tag, format, ##__VA_ARGS__)
    #else
      #define MLOGW(tag, format, ...)
    #endif

    #if (MATRIXOS_LOG_LEVEL >= LOG_LEVEL_INFO)
      #define MLOGI(tag, format, ...) MatrixOS::Logging::LogInfo(tag, format, ##__VA_ARGS__)
    #else
      #define MLOGI(tag, format, ...)
    #endif

    #if (MATRIXOS_LOG_LEVEL >= LOG_LEVEL_DEBUG)
      #define MLOGD(tag, format, ...) MatrixOS::Logging::LogDebug(tag, format, ##__VA_ARGS__)
    #else
      #define MLOGD(tag, format, ...)
    #endif

    #if (MATRIXOS_LOG_LEVEL >= LOG_LEVEL_VERBOSE)
      #define MLOGV(tag, format, ...) MatrixOS::Logging::LogVerbose(tag, format, ##__VA_ARGS__)
    #else
      #define MLOGV(tag, format, ...)
    #endif
  }

  namespace NVS
  {
    size_t GetSize(uint32_t hash);
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
}


// ui/UIInterface.h have more callable UI related function
