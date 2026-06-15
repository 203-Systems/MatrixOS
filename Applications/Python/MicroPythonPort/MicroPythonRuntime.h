#pragma once

#include <cstddef>
#include <string>

struct MicroPythonRuntimeStats {
  bool initialized = false;
  size_t heapSize = 0;
  size_t heapTotal = 0;
  size_t heapUsed = 0;
  size_t heapFree = 0;
  size_t heapMaxFree = 0;
  size_t gcOneBlockCount = 0;
  size_t gcTwoBlockCount = 0;
  size_t gcMaxBlockCount = 0;
  size_t cStackUsage = 0;
};

class MicroPythonRuntime {
public:
  MicroPythonRuntime() = default;

  void Init();
  void Deinit();
  bool Exec(const std::string& source, const char* sourceName = "<stdin>");
  bool ExecSingle(const std::string& source, const char* sourceName = "<stdin>");
  bool CallLoop();
  bool HasLoop() const;
  bool IsInitialized() const;
  MicroPythonRuntimeStats GetStats() const;

private:
  bool initialized = false;
  uintptr_t stackTop = 0;
  char heap[96 * 1024];

  bool ExecWithParseMode(const std::string& source, const char* sourceName, int parseMode);
};
