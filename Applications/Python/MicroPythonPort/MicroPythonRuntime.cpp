#include "MicroPythonRuntime.h"

#include "FreeRTOS.h"
#include "MatrixOS.h"

#if ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_debug_helpers.h"
#endif

extern "C" {
#include "py/compile.h"
#include "py/gc.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "port/micropython_embed.h"

void matrixos_micropython_set_script_path(const char* sourceName);
}

namespace
{
struct TrackedHeapAllocation {
  void* ptr = nullptr;
  size_t size = 0;
};

TrackedHeapAllocation autoHeapAllocations[MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS] = {};
size_t reservedHeapBytes = 0;

size_t GetLargestFreeHeapBlock() {
#if ESP_PLATFORM
  return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
#else
  return xPortGetFreeHeapSize();
#endif
}

size_t GetRemainingHeapBudget() {
  return reservedHeapBytes >= MATRIXOS_MICROPYTHON_HEAP_SIZE ? 0 : MATRIXOS_MICROPYTHON_HEAP_SIZE - reservedHeapBytes;
}

bool TrackAutoHeap(void* ptr, size_t size) {
  for (TrackedHeapAllocation& allocation : autoHeapAllocations)
  {
    if (allocation.ptr == nullptr)
    {
      allocation.ptr = ptr;
      allocation.size = size;
      return true;
    }
  }
  return false;
}

void AppendExceptionText(void* data, const char* text, size_t length) {
  if (data == nullptr || text == nullptr || length == 0)
  {
    return;
  }

  std::string* output = static_cast<std::string*>(data);
  constexpr size_t maxLoggedExceptionLength = 768;
  if (output->size() >= maxLoggedExceptionLength)
  {
    return;
  }

  size_t remaining = maxLoggedExceptionLength - output->size();
  output->append(text, length < remaining ? length : remaining);
}
} // namespace

extern "C" {
void* matrixos_micropython_alloc_heap(size_t size) {
  size_t remaining = GetRemainingHeapBudget();
  if (size == 0 || size > remaining)
  {
    MLOGW("Python", "MicroPython GC split allocation rejected: request=%d remaining=%d", static_cast<int>(size),
          static_cast<int>(remaining));
    return nullptr;
  }

  void* ptr = pvPortMalloc(size);
  if (ptr == nullptr)
  {
    MLOGW("Python", "MicroPython GC split allocation failed: request=%d largest=%d free=%d", static_cast<int>(size),
          static_cast<int>(GetLargestFreeHeapBlock()), static_cast<int>(xPortGetFreeHeapSize()));
    return nullptr;
  }

  if (!TrackAutoHeap(ptr, size))
  {
    MLOGW("Python", "MicroPython GC split allocation exceeded tracked chunks: request=%d", static_cast<int>(size));
    vPortFree(ptr);
    return nullptr;
  }

  reservedHeapBytes += size;
  MLOGD("Python", "Added MicroPython GC split heap chunk: %d bytes; reserved=%d", static_cast<int>(size),
        static_cast<int>(reservedHeapBytes));
  return ptr;
}

void matrixos_micropython_free_heap(void* ptr) {
  if (ptr == nullptr)
  {
    return;
  }

  for (TrackedHeapAllocation& allocation : autoHeapAllocations)
  {
    if (allocation.ptr == ptr)
    {
      reservedHeapBytes -= allocation.size;
      allocation = {};
      break;
    }
  }

  vPortFree(ptr);
}

size_t matrixos_micropython_get_max_new_split(void) {
  size_t remaining = GetRemainingHeapBudget();
  size_t largest = GetLargestFreeHeapBlock();
  size_t maxSplit = remaining < largest ? remaining : largest;
  return maxSplit < MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE ? maxSplit : MATRIXOS_MICROPYTHON_HEAP_CHUNK_SIZE;
}
}

MicroPythonRuntime::~MicroPythonRuntime() {
  Deinit();
  for (size_t i = 0; i < heapCount; i++)
  {
    reservedHeapBytes -= heapSizes[i];
    vPortFree(heaps[i]);
    heaps[i] = nullptr;
    heapSizes[i] = 0;
  }
  heapCount = 0;
}

bool MicroPythonRuntime::AllocateHeap() {
  if (heapCount > 0)
  {
    return true;
  }

  size_t initialSize = heapSize < heapChunkSize ? heapSize : heapChunkSize;
  uint8_t* heap = nullptr;
  while (initialSize >= MATRIXOS_MICROPYTHON_MIN_HEAP_CHUNK_SIZE)
  {
    heap = static_cast<uint8_t*>(pvPortMalloc(initialSize));
    if (heap != nullptr)
    {
      break;
    }
    initialSize /= 2;
  }

  if (heap == nullptr)
  {
    return false;
  }

  heaps[0] = heap;
  heapSizes[0] = initialSize;
  heapCount = 1;
  reservedHeapBytes += initialSize;
  MLOGD("Python", "Reserved initial MicroPython GC heap chunk: %d bytes", static_cast<int>(initialSize));
  return true;
}

bool MicroPythonRuntime::Init() {
  if (initialized)
  {
    return true;
  }

  if (!AllocateHeap())
  {
    return false;
  }

  mp_stack_ctrl_init();
  uintptr_t currentStackTop = 0;
  stackTop = (uintptr_t)&currentStackTop;
  void* heapAreas[MATRIXOS_MICROPYTHON_MAX_HEAP_CHUNKS] = {};
  for (size_t i = 0; i < heapCount; i++)
  {
    heapAreas[i] = heaps[i];
  }
  mp_embed_init_split(heapAreas, heapSizes, heapCount, (void*)stackTop);
  initialized = true;
  return true;
}

void MicroPythonRuntime::Deinit() {
  if (!initialized)
  {
    return;
  }

  gc_sweep_all();
  mp_embed_deinit();
  initialized = false;
}

bool MicroPythonRuntime::Exec(const std::string& source, const char* sourceName) {
  return ExecWithParseMode(source, sourceName, MP_PARSE_FILE_INPUT);
}

bool MicroPythonRuntime::ExecSingle(const std::string& source, const char* sourceName) {
  return ExecWithParseMode(source, sourceName, MP_PARSE_SINGLE_INPUT);
}

bool MicroPythonRuntime::ExecWithParseMode(const std::string& source, const char* sourceName, int parseMode) {
  if (!initialized)
  {
    return false;
  }

  lastExceptionType = "";
  lastExceptionText.clear();
  nlr_buf_t nlr;
  int nlrResult = nlr_push(&nlr);
  if (nlrResult == 0)
  {
    matrixos_micropython_set_script_path(sourceName);
    mp_lexer_t* lexer = mp_lexer_new_from_str_len(qstr_from_str(sourceName), source.c_str(), source.size(), 0);
    qstr lexerSourceName = lexer->source_name;
    mp_parse_tree_t parseTree = mp_parse(lexer, (mp_parse_input_kind_t)parseMode);
    mp_obj_t moduleFunction = mp_compile(&parseTree, lexerSourceName, true);
    mp_call_function_0(moduleFunction);
    nlr_pop();
    return true;
  }

  mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
  CaptureException(nlr.ret_val);
  LogCurrentBacktrace("MicroPython startup exception");
  return false;
}

bool MicroPythonRuntime::CallLoop() {
  if (!initialized)
  {
    return false;
  }

  mp_map_elem_t* loopElement = mp_map_lookup(mp_obj_dict_get_map(MP_OBJ_FROM_PTR(mp_globals_get())), MP_OBJ_NEW_QSTR(qstr_from_str("loop")),
                                             MP_MAP_LOOKUP);
  if (loopElement == nullptr)
  {
    return false;
  }

  lastExceptionType = "";
  lastExceptionText.clear();
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
  {
    mp_call_function_0(loopElement->value);
    nlr_pop();
    return true;
  }

  mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
  CaptureException(nlr.ret_val);
  LogCurrentBacktrace("MicroPython loop exception");
  return false;
}

bool MicroPythonRuntime::HasLoop() const {
  if (!initialized)
  {
    return false;
  }

  return mp_map_lookup(mp_obj_dict_get_map(MP_OBJ_FROM_PTR(mp_globals_get())), MP_OBJ_NEW_QSTR(qstr_from_str("loop")), MP_MAP_LOOKUP) != nullptr;
}

bool MicroPythonRuntime::IsInitialized() const {
  return initialized;
}

MicroPythonRuntimeStats MicroPythonRuntime::GetStats() const {
  MicroPythonRuntimeStats stats;
  stats.initialized = initialized;
  stats.heapSize = heapSize;

  if (!initialized)
  {
    return stats;
  }

  gc_info_t gcInfo = {};
  gc_info(&gcInfo);
  stats.heapTotal = gcInfo.total;
  stats.heapUsed = gcInfo.used;
  stats.heapFree = gcInfo.free;
  stats.heapMaxFree = gcInfo.max_free;
  stats.gcOneBlockCount = gcInfo.num_1block;
  stats.gcTwoBlockCount = gcInfo.num_2block;
  stats.gcMaxBlockCount = gcInfo.max_block;
  stats.cStackUsage = mp_stack_usage();
  return stats;
}

const char* MicroPythonRuntime::GetLastExceptionType() const {
  return lastExceptionType;
}

const char* MicroPythonRuntime::GetLastExceptionText() const {
  return lastExceptionText.c_str();
}

void MicroPythonRuntime::CaptureException(void* exception) {
  mp_obj_t exceptionObject = (mp_obj_t)exception;
  if (exceptionObject == MP_OBJ_NULL)
  {
    lastExceptionType = "<null>";
    lastExceptionText = "<null>";
    return;
  }

  lastExceptionType = mp_obj_is_exception_instance(exceptionObject) ? mp_obj_get_type_str(exceptionObject) : "<non-exception>";
  lastExceptionText.clear();
  mp_print_t exceptionPrint = {&lastExceptionText, AppendExceptionText};
  mp_obj_print_exception(&exceptionPrint, exceptionObject);
}

void MicroPythonRuntime::LogCurrentBacktrace(const char* reason) const {
#if ESP_PLATFORM
  uint32_t pc = 0;
  uint32_t sp = 0;
  uint32_t nextPc = 0;
  esp_backtrace_get_start(&pc, &sp, &nextPc);

  esp_backtrace_frame_t frame = {};
  frame.pc = pc;
  frame.sp = sp;
  frame.next_pc = nextPc;
  frame.exc_frame = nullptr;

  MLOGE("Python", "%s C backtrace start: pc=0x%08X sp=0x%08X next=0x%08X", reason, (unsigned int)frame.pc,
        (unsigned int)frame.sp, (unsigned int)frame.next_pc);

  for (int depth = 0; depth < 16 && frame.pc != 0; depth++)
  {
    MLOGE("Python", "  #%d pc=0x%08X sp=0x%08X next=0x%08X", depth, (unsigned int)frame.pc, (unsigned int)frame.sp,
          (unsigned int)frame.next_pc);
    if (!esp_backtrace_get_next_frame(&frame))
    {
      MLOGE("Python", "  backtrace stopped: corrupted frame");
      break;
    }
  }
#else
  (void)reason;
#endif
}
