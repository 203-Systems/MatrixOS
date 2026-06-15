#pragma once

#include "Application.h"
#include "MatrixOS.h"
#include "MicroPythonPort/MicroPythonRuntime.h"

class Python : public Application {
public:
  inline static Application_Info info = {
      .name = "Python",
      .author = "203 Systems",
      .color = Color(0x3776AB),
      .version = 1,
      .visibility = true,
  };

  Python();

  void Setup(const vector<string>& args) override;
  void Loop() override;
  void End() override;
  bool HasActiveLoop() const;
  bool IsReplMode() const;
  bool ShouldExitWhenIdle() const;
  const string& GetActiveScriptPath() const;
  MicroPythonRuntimeStats GetRuntimeStats() const;

private:
  MicroPythonRuntime runtime;
  bool hasLoop = false;
  bool replMode = false;
  bool exitWhenIdle = false;
  string activeScriptPath;
  string replBuffer;
  string replLineBuffer;
  bool replBlockMode = false;

  bool ExecutePythonFile(const string& filePath);
  bool ReadPythonFile(const string& filePath, string* output);
  void PollRepl();
  void PrintPrompt(bool continuation = false);
};

extern "C" {
uint32_t matrixos_python_input_available();
int matrixos_python_read_byte();
void matrixos_python_write_bytes(const char* data, uint32_t length);
void matrixos_python_notify_mode(uint8_t mode);
void matrixos_python_clear_input();
void matrixos_micropython_stdout(const char* data, unsigned int length);
const char* matrixos_python_get_runtime_debug_json();
}
