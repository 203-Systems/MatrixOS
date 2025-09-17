#pragma once

#include "MatrixOS.h"
#include "Application.h"

// Forward declaration
typedef struct PikaObj PikaObj;

class Python : public Application {
 public:
  inline static Application_Info info = {
      .name = "Python",
      .author = "203 Systems",
      .color = Color(0x3776AB), // Python blue
      .version = 1,
      .visibility = true,
  };

  PikaObj* pikaMain;

  void Setup(va_list args) override;
  void End() override;
};

// Platform functions for PikaPython
extern "C" {
  char pika_platform_getchar();
  int pika_platform_putchar(char ch);
  int64_t pika_platform_get_tick(void);
  void pika_platform_reboot(void);
  
  // PikaPython main functions
  PikaObj* pikaPythonInit(void);
  void pikaPythonShell(PikaObj* self);
}