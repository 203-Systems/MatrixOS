#ifndef __MATRIXOS_H
#define __MATRIXOS_H

#include "Device.h"
#include "Parameters.h"
#include "system/Variables.h"
#include "tusb.h"
#include "usb/MidiSpecs.h"
#include "framework/Framework.h"
// #include <string>

// using handler = void(*)();

#undef USB //CMSIS defined the USB, undef so we can use USB as namespace

// using std::string;

//All Public functions that are exposed to 
namespace MatrixOS
{
  namespace SYS
  {
    void Init(void);

    uint32_t Millis(void);
    void DelayMs(uint32_t intervalMs);

    void Reboot(void);
    void Bootloader(void);

    void SystemTask(void);

    enum class SysVar {
      //Device Info
      DeviceClass, DeviceName, DeviceVersion, DerviceRevision, SerialNumber,
      VelocityRange,
      LEDType, MatrixSizeX, MatrixSizeY, NumsOfLED, NumsOfKey,
      BootloaderVersion, SystemVersion, 

      //System variable
      Brightness, Rotation
      };
  
    uintptr_t GetAttribute(SysVar variable);
    int8_t SetAttribute(SysVar variable, uintptr_t value);

      // int Execute(uint32_t addr);

      void ErrorHandler(void);
  }

  namespace LED
  {
    #define LED_LAYERS 1
    extern uint8_t currentLayer;
    extern Color* frameBuffer;
    void Init(void);
    void SetColor(Point xy, Color color, uint8_t layer = currentLayer);
    void SetColor(uint32_t ID, Color color, uint8_t layer = currentLayer);
    void Fill(Color color, uint8_t layer = currentLayer);
    void Update(int8_t layer = currentLayer);
    void SwitchLayer(uint8_t layer);
  }

  namespace KEYPAD
  {
    void Init(void);

    enum EKeyStates {IDLE, PRESSED, ACTIVED,/* HOLD, HOLD_ACTIVED,*/ RELEASED, /*HOLD_RELEASED*/};

    struct KeyInfo {
      EKeyStates state = IDLE;
      uint32_t activeTime = 0;
      uint16_t velocity = 0;
      bool changed = false; //for Pressed, Hold, RELEASED, AFTERTOUCH
      bool hold = false;
      uint32_t holdTime(void)
      {
        if(state == IDLE)
        return 0;

        if(activeTime > SYS::Millis())
        return 0;

        return SYS::Millis() - activeTime;
      }
      operator bool() { return velocity > 0; }
    };

    extern void (*handler)(KeyInfo);
    void SetHandler(void (*handler)(KeyInfo));

    uint8_t Scan(void (*handler)(KeyInfo) = handler); //Return # of changed key, fetch changelist manually or pass in a callback as parameter
    // KeyInfo GetKey(Point keyXY);
    KeyInfo GetKey(uint16_t keyID);
  }

  // namespace DBG
  // {
  //   void Print (const char* format, ...);
  // }

  // namespace MEMORY
  // {
  //   const int SharedBufferSize = 2048+128;

  //   void SetSharedBuffer(void*);
  //   void* GetSharedBuffer(void);

  //   void LinearStart(void);
  //   bool LinearFinish(void);
  //   bool LinearProgram( uint32_t nAddress, unsigned char* pData, int nLength );
  // }

  // namespace FAT
  // {
  //   enum EIoMode {
  //     IoRead = 1,
  //     IoWrite = 2,
  //     IoClosed = 3
  //   };

  //   enum EResult 
  //   {
  //     EOk,
  //     EDiskError,
  //     EIntError,
  //     ENoFile,
  //     ENoPath,
  //     EDiskFull
  //   };

  //   enum EAttribute 
  //   {
  //     EReadOnly = 1,
  //     EHidden = 2,
  //     ESystem = 4,
  //     EDirectory = 0x10,
  //     EArchive = 0x20
  //   };

  //   struct TFindFile
  //   {
  //     uint32_t nFileLength;
  //     uint16_t nDate;
  //     uint16_t nTime;
  //     uint8_t nAtrib;
  //     char strName[13];
  //   };

  //   const int SectorSize = 4096;
  //   const int SectorCount = 2048;

  //   const int SharedBufferSize = SectorSize;

  //   void SetSharedBuffer(void*);
  //   void* GetSharedBuffer(void);

  //   EResult Init(void);
  //   EResult Open(const char* strName, uint8_t nIoMode);
  //   EResult Read(uint8_t* pSectorData);
  //   EResult Write(uint8_t* pSectorData);
  //   EResult Seek(uint32_t lOffset);
  //   EResult Close(int nSize);
  //   EResult Close(void);
  //   uint32_t GetFileSize(void);
	
  //   EResult OpenDir(char* strPath);
  //   EResult FindNext(TFindFile* pFile);
  // }

  // namespace OS
  // {
  //   typedef void (*TInterruptHandler)(void);
  //   enum EInterruptVector {
  //     IStackTop, IReset, INMIException, IHardFaultException, IMemManageException, 
  //     IBusFaultException, IUsageFaultException, _Dummy1, _Dummy2,
  //     _Dummy3, _Dummy4, ISVC, IDebugMonitor, _Dummy5, IPendSVC, 
  //     ISysTick, IWWDG_IRQ, IPVD_IRQ, ITAMPER_IRQ, IRTC_IRQ, IFLASH_IRQ,
  //     IRCC_IRQ, IEXTI0_IRQ, IEXTI1_IRQ, IEXTI2_IRQ, IEXTI3_IRQ, IEXTI4_IRQ,
  //     IDMA1_Channel1_IRQ, IDMA1_Channel2_IRQ, IDMA1_Channel3_IRQ,
  //     IDMA1_Channel4_IRQ, IDMA1_Channel5_IRQ, IDMA1_Channel6_IRQ,
  //     IDMA1_Channel7_IRQ, IADC1_2_IRQ, IUSB_HP_CAN_TX_IRQ, 
  //     IUSB_LP_CAN_RX0_IRQ, ICAN_RX1_IRQ, ICAN_SCE_IRQ, IEXTI9_5_IRQ,
  //     ITIM1_BRK_IRQ, ITIM1_UP_IRQ, ITIM1_TRG_COM_IRQ, ITIM1_CC_IRQ,
  //     ITIM2_IRQ, ITIM3_IRQ, ITIM4_IRQ, II2C1_EV_IRQ, II2C1_ER_IRQ,
  //     II2C2_EV_IRQ, II2C2_ER_IRQ, ISPI1_IRQ, ISPI2_IRQ, IUSART1_IRQ,
  //     IUSART2_IRQ, IUSART3_IRQ, IEXTI15_10_IRQ, IRTCAlarm_IRQ, 
  //     IUSBWakeUp_IRQ, ITIM8_BRK_IRQ, ITIM8_UP_IRQ, ITIM8_TRG_COM_IRQ,
  //     ITIM8_CC_IRQ, IADC3_IRQ, IFSMC_IRQ, ISDIO_IRQ, ITIM5_IRQ,
  //     ISPI3_IRQ, IUART4_IRQ, IUART5_IRQ, ITIM6_IRQ, ITIM7_IRQ,
  //     IDMA2_Channel1_IRQ, IDMA2_Channel2_IRQ, IDMA2_Channel3_IRQ,
  //     IDMA2_Channel4_5_IRQ };

  //   void SetArgument(char* argument);
  //   char* GetArgument(void);
  //   bool HasArgument(void);
  //   TInterruptHandler GetInterruptVector(EInterruptVector);
  //   void SetInterruptVector(EInterruptVector, TInterruptHandler);
  //   uint32_t DisableInterrupts(void);
  //   void EnableInterrupts(uint32_t);
  // }

  namespace USB
  {
    void Init(void);
    bool Inited(void); //If USB Stack is initlized, not sure what it will be needed but I added it anyways
    bool Connnected(void); //If USB is connected
    void Poll();

    namespace CDC
    {
      uint32_t Available(void);
      void Poll(void);
      
      void Print(char const* str);
      void Println(char const* str);

      extern void (*handler)(char const*);
      void Read(void);
      void SetHandler(void (*handler)(char const*));
    }
    
    namespace MIDI
    {
      void Init(void);
      void Poll(void);

      enum Status{NoteOff,
                  NoteOn,
                  AfterTouch,
                  ControlChange,
                  ProgramChange,
                  ChannelPressure,
                  PitchChange,
                  SongPosition,
                  SongSelect,
                  TuneRequest,
                  Sync,
                  Start,
                  Continue,
                  Stop,
                  ActiveSense,
                  Reset,
                  SysexData,
                  SysexEnd,
                  HandlerCount
                  };


      extern void (* handlers[HandlerCount])();

      void SetHandler(Status status, handler handler);
      void ClearHandler(Status status);
      void ClearAllHandler(void);
      void CallHandler(Status status, uint32_t value1 = 0, uint32_t value2 = 0, uint32_t value3 = 0);

      void DispatchPacket(uint8_t packet[4]);

      void SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
      void SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
      void SendAfterTouch(uint8_t channel, uint8_t note, uint8_t velocity);
      void SendControlChange(uint8_t channel, uint8_t controller, uint8_t value);
      void SendProgramChange(uint8_t channel, uint8_t program);
      void SendChannelPressure(uint8_t channel, uint8_t velocity);
      void SendPitchChange(uint8_t channel, uint16_t pitch);
      void SendSongPosition(uint16_t position);
      void SendSongSelect(uint8_t song);
      void SendTuneRequest(void);
      void SendSync(void);
      void SendStart(void);
      void SendContinue(void);
      void SendStop(void);
      void SendActiveSense(void);
      void SendReset(void);
    }

    // void Disable(void);
  }

  // namespace GPIO
  // {
  //   enum EPin {P1, P2, P3, P4, P5, P6, P7, P8};
  //   enum EMode {Input = 1, Output = 2, Pwm = 4, PullUp = 8, PullDown = 16, I2c = 32, Uart = 64};
  //   const int AnalogRange = 1024;

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