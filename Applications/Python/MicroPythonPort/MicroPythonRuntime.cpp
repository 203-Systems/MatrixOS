#include "MicroPythonRuntime.h"

extern "C" {
#include "py/compile.h"
#include "py/gc.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "port/micropython_embed.h"

void matrixos_micropython_set_script_path(const char* sourceName);
}

void MicroPythonRuntime::Init() {
  if (initialized)
  {
    return;
  }

  mp_stack_ctrl_init();
  uintptr_t currentStackTop = 0;
  stackTop = (uintptr_t)&currentStackTop;
  mp_embed_init(heap, sizeof(heap), (void*)stackTop);
  initialized = true;
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

  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
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

  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
  {
    mp_call_function_0(loopElement->value);
    nlr_pop();
    return true;
  }

  mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
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
  stats.heapSize = sizeof(heap);

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
