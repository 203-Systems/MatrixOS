#include "Python.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "System/System.h"

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


void Python::Setup(va_list args) {
  // Flush serial RX buffer
  while(MatrixOS::USB::CDC::Available())
  {
    (void)MatrixOS::USB::CDC::Read();
  }

  // Print Matrix OS ASCII art banner
  MatrixOS::USB::CDC::Println("");
  MatrixOS::USB::CDC::Println("███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗     ██████╗ ███████╗");
  MatrixOS::USB::CDC::Println("████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝    ██╔═══██╗██╔════╝");
  MatrixOS::USB::CDC::Println("██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝     ██║   ██║███████╗");
  MatrixOS::USB::CDC::Println("██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗     ██║   ██║╚════██║");
  MatrixOS::USB::CDC::Println("██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗    ╚██████╔╝███████║");
  MatrixOS::USB::CDC::Println("╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝");
  MatrixOS::USB::CDC::Println("Matrix OS Python REPL");
  MatrixOS::USB::CDC::Println("OS Version: " + MATRIXOS_VERSION_STRING);
  MatrixOS::USB::CDC::Println("See matrix.203.io for docs and usage guide.");
  MatrixOS::USB::CDC::Println("");

  // Remove previlige from application
  MatrixOS::SYS::SetTaskPermissions(MatrixOS::SYS::TaskPermissions());

  // Initialize PikaPython
  pikaMain = pikaPythonInit();
  pikaPythonShell(pikaMain);
  
  Exit();
}

void Python::End()
{
  // Deinitialize PikaPython after shell exits
  obj_deinit(pikaMain);
}