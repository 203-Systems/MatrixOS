#include "Python.h"

#include "pika_config.h"
#include "pikaScript.h"
#include "PikaObj.h"

namespace MatrixOS::USB::CDC
{
  void WriteChar(char c, void* arg);
}

// Platform abstraction functions for PikaPython
extern "C" {
  char pika_platform_getchar() {
    while (!MatrixOS::USB::CDC::Available()) {
      MatrixOS::SYS::DelayMs(1); // Yield to other tasks
    }
    return MatrixOS::USB::CDC::Read();
  }

  int pika_platform_putchar(char ch) {
    MatrixOS::USB::CDC::WriteChar(ch, nullptr);
    MatrixOS::USB::CDC::Flush(); // Ensure immediate output
    return 0;
  }

  int64_t pika_platform_get_tick(void) {
    return MatrixOS::SYS::Millis();
  }

  void pika_platform_reboot(void) {
    // Reboot the system
    MatrixOS::SYS::Reboot();
  }
}

void Python::Setup() {
  // Flush serial RX buffer
  while(MatrixOS::USB::CDC::Available())
  {
    (void)MatrixOS::USB::CDC::Read();
  }

  // Initialize PikaPython
  PikaObj* pikaMain = pikaPythonInit();
  pikaPythonShell(pikaMain);

  // Deinitialize PikaPython after shell exits
  extern volatile PikaObj *__pikaMain;
  if (pikaMain != nullptr) {
    obj_deinit(pikaMain);
    __pikaMain = nullptr;
  }
  
  Exit();
}