#pragma once

#include <stddef.h>
#include <port/mpconfigport_common.h>

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES)

#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_GC_SPLIT_HEAP (1)
#define MICROPY_GC_SPLIT_HEAP_AUTO (1)
#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_GCREGS_SETJMP (1)
#define MICROPY_NLR_SETJMP (1)
#define MICROPY_ENABLE_SOURCE_LINE (1)
#define MICROPY_HAS_FILE_READER (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_REPL_EVENT_DRIVEN (0)
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_LONGLONG)

#define MICROPY_PERSISTENT_CODE_LOAD (0)
#define MICROPY_PY_BUILTINS_HELP (1)
#define MICROPY_PY_BUILTINS_EXECFILE (0)
#define MICROPY_PY_GC (1)
#define MICROPY_PY_IO (1)
#define MICROPY_PY_SYS (1)
#define MICROPY_PY_SYS_EXIT (1)
#define MICROPY_PY_SYS_PLATFORM "matrixos"
#define MICROPY_PY_TIME (0)

#if defined(ESP_PLATFORM) && defined(__xtensa__)
#undef MICROPY_PERSISTENT_CODE_LOAD
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#define MICROPY_EMIT_INLINE_XTENSA (1)
#define MICROPY_EMIT_INLINE_XTENSA_UNCOMMON_OPCODES (1)
#if defined(__XTENSA_WINDOWED_ABI__)
#define MICROPY_EMIT_XTENSAWIN (1)
#else
#define MICROPY_EMIT_XTENSA (1)
#endif
#define MICROPY_OPT_COMPUTED_GOTO (1)
#define MICROPY_OPT_LOAD_ATTR_FAST_PATH (1)
#define MICROPY_OPT_MAP_LOOKUP_CACHE (1)
#define MP_PLAT_COMMIT_EXEC(buf, len, reloc) matrixos_micropython_commit_exec((buf), (len), (reloc))
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void matrixos_micropython_stdout(const char* data, unsigned int length);
extern void* matrixos_micropython_commit_exec(void* buf, size_t len, void* reloc);
extern void* matrixos_micropython_alloc_heap(size_t size);
extern void matrixos_micropython_free_heap(void* ptr);
extern size_t matrixos_micropython_get_max_new_split(void);
#ifdef __cplusplus
}
#endif
#define MP_PLAT_PRINT_STRN(str, len) matrixos_micropython_stdout((str), (unsigned int)(len))
#define MP_PLAT_ALLOC_HEAP(size) matrixos_micropython_alloc_heap(size)
#define MP_PLAT_FREE_HEAP(ptr) matrixos_micropython_free_heap(ptr)
