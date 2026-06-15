#include "Python.h"

#include "System/System.h"

#if DEVICE_STORAGE == 1
#include "FileSystem/FileSystem.h"
#endif

#if MATRIXOS_WEB
#include "HostIO.h"
#endif

#if ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

extern "C" {
#include "py/repl.h"
}

namespace
{
Python* activePythonInstance = nullptr;
string pythonRuntimeDebugJson = "{\"active\":false,\"runtime\":{\"initialized\":false,\"heapSize\":0}}";

void PythonPrint(const string& text) {
  matrixos_python_write_bytes(text.c_str(), static_cast<uint32_t>(text.size()));
}

void PythonPrintln(const string& text) {
  PythonPrint(text);
  static const char newline[] = "\n\r";
  matrixos_python_write_bytes(newline, 2);
}

void PrintBanner() {
  PythonPrintln("");
  PythonPrintln("Matrix OS MicroPython");
  PythonPrintln("OS Version: " + MATRIXOS_VERSION_STRING);
  PythonPrintln("");
}

bool IsBlankLine(const string& line) {
  for (char character : line)
  {
    if (character != ' ' && character != '\t')
    {
      return false;
    }
  }
  return true;
}

bool IsIdentifierBoundary(char character) {
  return !((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_');
}

bool StartsWithWord(const string& text, size_t offset, const char* word) {
  size_t index = 0;
  while (word[index] != '\0')
  {
    if (offset + index >= text.size() || text[offset + index] != word[index])
    {
      return false;
    }
    index++;
  }

  return offset + index >= text.size() || IsIdentifierBoundary(text[offset + index]);
}

bool StartsCompoundBlock(const string& text) {
  size_t offset = 0;
  while (offset < text.size() && (text[offset] == ' ' || text[offset] == '\t'))
  {
    offset++;
  }

  if (offset >= text.size())
  {
    return false;
  }

  if (text[offset] == '@')
  {
    return true;
  }

  static const char* compoundWords[] = {"if", "while", "for", "try", "with", "def", "class", "async"};
  for (const char* word : compoundWords)
  {
    if (StartsWithWord(text, offset, word))
    {
      return true;
    }
  }

  return false;
}

string BoolJson(bool value) {
  return value ? "true" : "false";
}

string JsonEscape(const string& value) {
  string escaped;
  escaped.reserve(value.size() + 8);
  for (char character : value)
  {
    switch (character)
    {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += character;
        break;
    }
  }
  return escaped;
}

string BuildRuntimeStatsJson(const MicroPythonRuntimeStats& stats) {
  string json = "{";
  json += "\"initialized\":" + BoolJson(stats.initialized) + ",";
  json += "\"heapSize\":" + std::to_string(stats.heapSize) + ",";
  json += "\"heapTotal\":" + std::to_string(stats.heapTotal) + ",";
  json += "\"heapUsed\":" + std::to_string(stats.heapUsed) + ",";
  json += "\"heapFree\":" + std::to_string(stats.heapFree) + ",";
  json += "\"heapMaxFree\":" + std::to_string(stats.heapMaxFree) + ",";
  json += "\"gcOneBlockCount\":" + std::to_string(stats.gcOneBlockCount) + ",";
  json += "\"gcTwoBlockCount\":" + std::to_string(stats.gcTwoBlockCount) + ",";
  json += "\"gcMaxBlockCount\":" + std::to_string(stats.gcMaxBlockCount) + ",";
  json += "\"cStackUsage\":" + std::to_string(stats.cStackUsage);
  json += "}";
  return json;
}

string BuildPythonRuntimeDebugJson() {
  Python* python = activePythonInstance;
  string json = "{";
  json += "\"active\":" + BoolJson(python != nullptr) + ",";
  json += "\"hasLoop\":" + BoolJson(python != nullptr && python->HasActiveLoop()) + ",";
  json += "\"replMode\":" + BoolJson(python != nullptr && python->IsReplMode()) + ",";
  json += "\"exitWhenIdle\":" + BoolJson(python != nullptr && python->ShouldExitWhenIdle()) + ",";
  json += "\"scriptPath\":\"" + JsonEscape(python != nullptr ? python->GetActiveScriptPath() : "") + "\",";
  json += "\"runtime\":";
  json += BuildRuntimeStatsJson(python != nullptr ? python->GetRuntimeStats() : MicroPythonRuntimeStats{});
  json += "}";
  return json;
}

size_t GetLargestRuntimeHeapBlock() {
#if ESP_PLATFORM
  return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
#else
  return xPortGetFreeHeapSize();
#endif
}
}

Python::Python() {
  runtime.AllocateHeap();
}

extern "C" {
__attribute__((weak)) uint32_t matrixos_python_input_available() {
  return MatrixOS::USB::CDC::Available();
}

__attribute__((weak)) int matrixos_python_read_byte() {
  return MatrixOS::USB::CDC::Read();
}

__attribute__((weak)) void matrixos_python_write_bytes(const char* data, uint32_t length) {
  if (data == nullptr || length == 0)
  {
    return;
  }

  MatrixOS::USB::CDC::Print(string(data, length));
  MatrixOS::USB::CDC::Flush();
}

__attribute__((weak)) void matrixos_python_notify_mode(uint8_t mode) {
  (void)mode;
}

__attribute__((weak)) void matrixos_python_clear_input() {
  while (MatrixOS::USB::CDC::Available())
  {
    (void)MatrixOS::USB::CDC::Read();
  }
}

void matrixos_micropython_stdout(const char* data, unsigned int length) {
  matrixos_python_write_bytes(data, length);
}

const char* matrixos_python_get_runtime_debug_json() {
  pythonRuntimeDebugJson = BuildPythonRuntimeDebugJson();
  return pythonRuntimeDebugJson.c_str();
}
}

void Python::Setup(const vector<string>& args) {
  activePythonInstance = this;
  MLOGI("Python", "Setup start: args=%d free heap=%d largest block=%d", args.size(), xPortGetFreeHeapSize(), GetLargestRuntimeHeapBlock());
  matrixos_python_clear_input();
  if (!runtime.Init())
  {
    MLOGE("Python",
          "Failed to allocate MicroPython GC heap (%d bytes, chunk %d bytes); free heap: %d bytes; largest block: %d bytes",
          MATRIXOS_MICROPYTHON_HEAP_SIZE, MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE, xPortGetFreeHeapSize(), GetLargestRuntimeHeapBlock());
    Exit();
    return;
  }
  MicroPythonRuntimeStats stats = runtime.GetStats();
  MLOGD("Python", "MicroPython runtime initialized: heap budget %d bytes, chunk %d bytes, gc total %d bytes, free heap %d bytes",
        MATRIXOS_MICROPYTHON_HEAP_SIZE, MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE, stats.heapTotal, xPortGetFreeHeapSize());
  replMode = false;
  exitWhenIdle = false;
  activeScriptPath.clear();
  replBuffer.clear();
  replLineBuffer.clear();
  replBlockMode = false;

  if (!args.empty())
  {
    matrixos_python_notify_mode(2);
    const string& pythonFilePath = args[0];
    activeScriptPath = pythonFilePath;
    MLOGI("Python", "Executing MicroPython script: %s", pythonFilePath.c_str());

    if (ExecutePythonFile(pythonFilePath))
    {
      hasLoop = runtime.HasLoop();
      MLOGI("Python", "MicroPython script executed successfully");
      if (!hasLoop)
      {
        MicroPythonRuntimeStats stats = runtime.GetStats();
        MLOGW("Python", "MicroPython script has no loop(); exiting idle script. gc total=%d used=%d free=%d max_free=%d",
              stats.heapTotal, stats.heapUsed, stats.heapFree, stats.heapMaxFree);
        exitWhenIdle = true;
      }
      return;
    }

    MLOGE("Python", "Failed to execute MicroPython script: %s", pythonFilePath.c_str());
    Exit();
    return;
  }

  matrixos_python_notify_mode(1);
  PrintBanner();
  replMode = true;
  PrintPrompt();
}

bool Python::HasActiveLoop() const {
  return hasLoop;
}

bool Python::IsReplMode() const {
  return replMode;
}

bool Python::ShouldExitWhenIdle() const {
  return exitWhenIdle;
}

const string& Python::GetActiveScriptPath() const {
  return activeScriptPath;
}

MicroPythonRuntimeStats Python::GetRuntimeStats() const {
  return runtime.GetStats();
}

void Python::Loop() {
  if (replMode)
  {
    PollRepl();
    return;
  }

  if (!hasLoop)
  {
    if (exitWhenIdle)
    {
      Exit();
    }
    return;
  }

  if (!runtime.CallLoop())
  {
    MicroPythonRuntimeStats stats = runtime.GetStats();
    MLOGE("Python", "MicroPython loop failed: exception=%s; gc total=%d used=%d free=%d max_free=%d; system free=%d; traceback=%s",
          runtime.GetLastExceptionType(), stats.heapTotal, stats.heapUsed, stats.heapFree, stats.heapMaxFree, xPortGetFreeHeapSize(),
          runtime.GetLastExceptionText());
    hasLoop = false;
    Exit();
    return;
  }

  MatrixOS::SYS::DelayMs(1);
}

void Python::PollRepl() {
  while (matrixos_python_input_available())
  {
    int input = matrixos_python_read_byte();
    if (input < 0)
    {
      return;
    }

    char character = static_cast<char>(input);
    if (character == 0x04)
    {
      PythonPrintln("");
      Exit();
      return;
    }

    if (character == 0x03)
    {
      replBuffer.clear();
      replLineBuffer.clear();
      replBlockMode = false;
      PythonPrintln("^C");
      PrintPrompt();
      continue;
    }

    if (character == '\b' || character == 0x7F)
    {
      if (!replLineBuffer.empty())
      {
        replBuffer.pop_back();
        replLineBuffer.pop_back();
        PythonPrint("\b \b");
      }
      continue;
    }

    if (character == '\r' || character == '\n')
    {
      PythonPrintln("");
      bool blankLine = IsBlankLine(replLineBuffer);
      if (replBuffer.empty() && blankLine)
      {
        replLineBuffer.clear();
        PrintPrompt();
        continue;
      }

      replBuffer.push_back('\n');
      replBlockMode = replBlockMode || StartsCompoundBlock(replBuffer);
      bool needsMoreInput = mp_repl_continue_with_input(replBuffer.c_str());
      bool shouldExecute = !needsMoreInput && (!replBlockMode || blankLine);

      if (shouldExecute)
      {
        runtime.ExecSingle(replBuffer, "<stdin>");
        replBuffer.clear();
        replLineBuffer.clear();
        replBlockMode = false;
        PrintPrompt();
      }
      else
      {
        replLineBuffer.clear();
        PrintPrompt(true);
      }
      continue;
    }

    if (character >= ' ' && character <= '~')
    {
      replBuffer.push_back(character);
      replLineBuffer.push_back(character);
      matrixos_python_write_bytes(&character, 1);
    }
  }
}

void Python::PrintPrompt(bool continuation) {
  PythonPrint(continuation ? "... " : ">>> ");
}

bool Python::ExecutePythonFile(const string& filePath) {
  string source;
  if (!ReadPythonFile(filePath, &source))
  {
    MLOGE("Python", "Failed to read MicroPython script: %s", filePath.c_str());
    return false;
  }

  MLOGI("Python", "Loaded MicroPython script: %s (%d bytes)", filePath.c_str(), source.size());
  bool ok = runtime.Exec(source, filePath.c_str());
  if (!ok)
  {
    MicroPythonRuntimeStats stats = runtime.GetStats();
    MLOGE("Python", "MicroPython script startup failed: %s; exception=%s; gc total=%d used=%d free=%d max_free=%d; system free=%d; traceback=%s",
          filePath.c_str(), runtime.GetLastExceptionType(), stats.heapTotal, stats.heapUsed, stats.heapFree, stats.heapMaxFree,
          xPortGetFreeHeapSize(), runtime.GetLastExceptionText());
  }
  return ok;
}

bool Python::ReadPythonFile(const string& filePath, string* output) {
  if (output == nullptr)
  {
    return false;
  }

#if MATRIXOS_WEB
  if (MystrixSim::HostIO::GetPythonScript(filePath, output))
  {
    MLOGD("Python", "Loaded staged host MicroPython script: %s", filePath.c_str());
    return true;
  }
#endif

#if DEVICE_STORAGE == 1
  File file = MatrixOS::FileSystem::Open(filePath, "r");
  if (!file.Available() && file.Size() == 0)
  {
    return false;
  }

  size_t size = file.Size();
  output->resize(size);
  size_t bytesRead = file.Read(output->data(), size);
  output->resize(bytesRead);
  file.Close();
  if (bytesRead != size)
  {
    MLOGW("Python", "MicroPython script read partial: %s (%d/%d bytes)", filePath.c_str(), bytesRead, size);
  }
  return bytesRead > 0 || size == 0;
#else
  (void)filePath;
  return false;
#endif
}

void Python::End() {
  matrixos_python_notify_mode(0);
  hasLoop = false;
  replMode = false;
  exitWhenIdle = false;
  activeScriptPath.clear();
  replBuffer.clear();
  replLineBuffer.clear();
  replBlockMode = false;
  runtime.Deinit();
  if (activePythonInstance == this)
  {
    activePythonInstance = nullptr;
  }
}
