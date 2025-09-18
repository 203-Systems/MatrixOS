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


void Python::Setup(const vector<string>& args) {
  // Flush serial RX buffer
  while(MatrixOS::USB::CDC::Available())
  {
    (void)MatrixOS::USB::CDC::Read();
  }

  // Check if a script path was provided
  if (!args.empty()) {
    const string& python_file_path = args[0];
    MLOGI("Python", "Executing Python script: %s", python_file_path.c_str());

    // Execute the Python script file
    if (ExecutePythonFile(python_file_path)) {
      MLOGI("Python", "Python script executed successfully");
    } else {
      MLOGE("Python", "Failed to execute Python script: %s", python_file_path.c_str());
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

bool Python::ExecutePythonFile(const string& file_path) {
#if DEVICE_STORAGE == 1
  MLOGD("Python", "Attempting to execute Python file: %s", file_path.c_str());

  // Create pikascript-api directory for compiled output
  size_t last_slash = file_path.find_last_of('/');
  if (last_slash != string::npos) {
    string script_dir = file_path.substr(0, last_slash);
    string api_dir = script_dir + "/pikascript-api";

    // Check if directory exists, create if not
    if (!MatrixOS::FileSystem::Exists(api_dir)) {
      if (!MatrixOS::FileSystem::MakeDir(api_dir)) {
        return false;
      }
    }
  }

  pikaVM_runFile(pikaMain, (char*)file_path.c_str());

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