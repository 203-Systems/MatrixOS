#ifndef __MATRIXOS_H
#define __MATRIXOS_H

#include "Device.h"
#include "system/Parameters.h"
#include "system/UserVariables.h"
#include "system/SystemVariables.h"
#include "tusb.h"
#include "usb/MidiSpecs.h"
#include "framework/Framework.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#define noexpose //Custum keyword to remove function to be generated as exposed API

class Application;

//Matrix OS Modules and their API for Application layer or system layer
namespace MatrixOS
{
  inline uint32_t API_version = 0;
  namespace SYS
  {
    inline bool inited = false;
    void Init(void);

    uint32_t Millis(void);
    void DelayMs(uint32_t intervalMs);

    void Reboot(void);
    void Bootloader(void);

    void OpenSetting(void);

    enum class EVarClass {DeviceVar, SystemVar, UserVar, AppVar};
    uint32_t GetVariable(string variable, EVarClass varClass = EVarClass::UserVar);
    int8_t SetVariable(string variable, uint32_t value);

    void Rotate(EDirection rotation, bool absolute = false);
    void NextBrightness();
    
    void ExecuteAPP(uint32_t app_id);
    void ExitAPP();
    
    // int Execute(uint32_t addr);

    void ErrorHandler(string error = NULL);
  }

  namespace LED
  {
    inline uint8_t currentLayer;
    inline vector<Color*> frameBuffers;
    void Init(void);
    void SetColor(Point xy, Color color, uint8_t layer = currentLayer);
    void SetColor(uint16_t ID, Color color, uint8_t layer = currentLayer);
    void Fill(Color color, uint8_t layer = currentLayer);
    void Update(int8_t layer = currentLayer);
    // void PauseAutoUpdate();
    // void StartAutoUpdate();
    void SwitchLayer(uint8_t layer);

    int8_t CreateLayer();
    void DestoryLayer();
    
    void ShiftCanvas(EDirection direction, int8_t distance, int8_t layer = currentLayer);
    void RotateCanvas(EDirection direction, int8_t layer = currentLayer);
  }

  namespace KEYPAD
  {
    noexpose void Init(void);

    // extern void (*handler)(uint16_t);
    // void SetHandler(void (*handler)(uint16_t));

    uint16_t Scan(void); //Return # of changed key, fetch changelist manually or pass in a callback as parameter
    uint16_t Available();
    uint16_t Get();
    KeyInfo GetKey(Point keyXY);
    KeyInfo GetKey(uint16_t keyID);
    void Clear();
    uint16_t XY2ID(Point xy); //Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID is assigned to given XY
    Point ID2XY(uint16_t keyID); //Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for given ID;

  }

  namespace USB
  {
    noexpose void Init();
    bool Inited(void); //If USB Stack is initlized, not sure what it will be needed but I added it anyways
    bool Connected(void); //If USB is connected
    // void Poll(); 

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
      uint32_t ReadBytes(void* buffer, uint32_t length); //Returns nums byte read
      string ReadString(void);
    }
  }

  namespace MIDI
    {
      noexpose void Init(void);
      // void Poll(void); //Not intented for app use. called from SystemTask()

      uint32_t Available();
      MidiPacket Get();

      //USB
      noexpose MidiPacket GetUSB();
      noexpose MidiPacket DispatchUSBPacket(uint8_t packet[4]);

      void SendPacket(MidiPacket midiPacket);
      // void SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
      // void SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
      // void SendAfterTouch(uint8_t channel, uint8_t note, uint8_t velocity);
      // void SendControlChange(uint8_t channel, uint8_t controller, uint8_t value);
      // void SendProgramChange(uint8_t channel, uint8_t program);
      // void SendChannelPressure(uint8_t channel, uint8_t velocity);
      // void SendPitchChange(uint8_t channel, uint16_t pitch);
      // void SendSongPosition(uint16_t position);
      // void SendSongSelect(uint8_t song);
      // void SendTuneRequest(void);
      // void SendSync(void);
      // void SendStart(void);
      // void SendContinue(void);
      // void SendStop(void);
      // void SendActiveSense(void);
      // void SendReset(void);

      // extern void (*handler)(MidiPacket);
      // void SetHandler(void (*new_handler)(MidiPacket));
    }

  namespace Logging
  {
    void LogLevelSet(string tag, ELogLevel level);
    void LogError (string tag, string format, ...);
    void LogWarning (string tag, string format, ...);
    void LogInfo (string tag, string format, ...);
    void LogDebug (string tag, string format, ...);
    void LogVerbose (string tag, string format, ...);
  }

  namespace NVS
  {
    vector<char> GetVariable(string name);
    int8_t GetVariable(string name, void* pointer, uint16_t length); //Load variable into pointer. If not defined, it will try to assign current pointer value into it.
    bool SetVariable(string name, void* pointer, uint16_t length);
    bool DeleteVariable(string name);
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
  //     enum EConfigMask {length = EConfig::length8 | EConfig::length9, stopBits = EConfig::stopBits1 | EConfig::stopBits15 |
  //         EConfig::stopBits2, parity = EConfig::parityNone | EConfig::parityEven | EConfig::parityOdd, flow = EConfig::flowNone | EConfig::flowHw
  //     };
  //     void Setup(int baudrate, EConfig config);
  //     bool Available(void);
  //     uint8_t Read(void);
  //     void Write(uint8_t);
  //   }
}

#endif