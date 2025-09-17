#include "Python.h"
#include "pikaScript.h"
#include "pikaVM.h"
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

  // Check if a Python file path was provided as argument
  char* python_file_path = va_arg(args, char*);

  if (python_file_path != nullptr) {
    MLOGI("Python", "Executing Python script: %s", python_file_path);

    // Execute the Python script file
    if (ExecutePythonFile(python_file_path)) {
      MLOGI("Python", "Python script executed successfully");
    } else {
      MLOGE("Python", "Failed to execute Python script: %s", python_file_path);
    }
  } else {
    // No file specified, start REPL mode
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

    pikaPythonShell(pikaMain);
  }

  Exit();
}

bool Python::ExecutePythonFile(char* file_path) {
#if DEVICE_STORAGE == 1
  MLOGD("Python", "Attempting to execute Python file: %s", file_path);

  pikaVM_runFile(pikaMain, file_path);

  return true;
#else
  MLOGE("Python", "Filesystem not available, cannot execute Python file");
  return false;
#endif
}

void Python::End()
{
  // Deinitialize PikaPython after shell exits
  obj_deinit(pikaMain);
}