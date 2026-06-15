#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#ifndef MATRIXOS_MICROPYTHON_HEAP_SIZE
#if MATRIXOS_WEB
#define MATRIXOS_MICROPYTHON_HEAP_SIZE (96 * 1024)
#else
#define MATRIXOS_MICROPYTHON_HEAP_SIZE (48 * 1024)
#endif
#endif

#ifndef MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE
#if MATRIXOS_WEB
#define MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE MATRIXOS_MICROPYTHON_HEAP_SIZE
#else
#define MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE (16 * 1024)
#endif
#endif

#ifndef MATRIXOS_MICROPYTHON_MIN_HEAP_CHUNK_SIZE
#define MATRIXOS_MICROPYTHON_MIN_HEAP_CHUNK_SIZE (8 * 1024)
#endif

#ifndef MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS
#define MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS 8
#endif

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
  ~MicroPythonRuntime();

  bool AllocateHeap();
  bool Init();
  void Deinit();
  bool Exec(const std::string& source, const char* sourceName = "<stdin>");
  bool ExecSingle(const std::string& source, const char* sourceName = "<stdin>");
  bool CallLoop();
  bool HasLoop() const;
  bool IsInitialized() const;
  MicroPythonRuntimeStats GetStats() const;
  const char* GetLastExceptionType() const;
  const char* GetLastExceptionText() const;

private:
  bool initialized = false;
  const char* lastExceptionType = "";
  std::string lastExceptionText;
  uintptr_t stackTop = 0;
  uint8_t* heaps[MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS] = {};
  size_t heapSizes[MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS] = {};
  size_t heapCount = 0;
  size_t heapSize = MATRIXOS_MICROPYTHON_HEAP_SIZE;
  size_t heapChunkSize = MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE;

  bool ExecWithParseMode(const std::string& source, const char* sourceName, int parseMode);
  void CaptureException(void* exception);
  void LogCurrentBacktrace(const char* reason) const;
};
