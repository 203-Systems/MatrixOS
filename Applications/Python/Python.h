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

  void Setup(const vector<string>& args) override;
  void End() override;

private:
  bool ExecutePythonFile(const string& file_path);
};

// Platform functions for PikaPython
extern "C" {
  char pika_platform_getchar();
  int pika_platform_putchar(char ch);
  int64_t pika_platform_get_tick(void);
  void pika_platform_reboot(void);

  // File I/O platform functions
  FILE* pika_platform_fopen(const char* filename, const char* modes);
  size_t pika_platform_fwrite(const void* ptr, size_t size, size_t n, FILE* stream);
  size_t pika_platform_fread(void* ptr, size_t size, size_t n, FILE* stream);
  int pika_platform_fclose(FILE* stream);
  int pika_platform_fseek(FILE* stream, long offset, int whence);
  long pika_platform_ftell(FILE* stream);

  // Directory and path functions
  char* pika_platform_getcwd(char* buf, size_t size);
  int pika_platform_chdir(const char* path);
  int pika_platform_rmdir(const char* pathname);
  int pika_platform_mkdir(const char* pathname, int mode);
  int pika_platform_remove(const char* pathname);
  int pika_platform_rename(const char* oldpath, const char* newpath);
  char** pika_platform_listdir(const char* path, int* count);
  int pika_platform_path_exists(const char* path);
  int pika_platform_path_isdir(const char* path);
  int pika_platform_path_isfile(const char* path);

  // PikaPython main functions
  PikaObj* pikaPythonInit(void);
  void pikaPythonShell(PikaObj* self);
}