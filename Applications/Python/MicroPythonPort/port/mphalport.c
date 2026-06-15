/*
 * MatrixOS MicroPython stdout bridge.
 */

#include "py/mphal.h"

extern void matrixos_micropython_stdout(const char* data, unsigned int length);

void mp_hal_stdout_tx_strn_cooked(const char* str, size_t len) {
  matrixos_micropython_stdout(str, (unsigned int)len);
}
