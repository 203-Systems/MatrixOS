#include "Python.h"
#include "pikaScript.h"
#include "PikaVM.h"
#include "PikaObj.h"
#include "System/System.h"
#include "FileSystem/FatFS/ff.h"

namespace MatrixOS::USB::CDC
{
void WriteChar(char c, void* arg);
}

// Platform abstraction functions for PikaPython
extern "C" {
char pika_platform_getchar() {
  while (!MatrixOS::USB::CDC::Available())
  {
    MatrixOS::SYS::DelayMs(1); // Yield to other tasks
  }
  return MatrixOS::USB::CDC::Read();
}

int pika_platform_putchar(char ch) {
  MatrixOS::USB::CDC::WriteChar(ch, nullptr);
  MatrixOS::USB::CDC::Flush(); // Ensure immediate output
  return 0;
}

int64_t pika_platform_get_tick(void) {
  return MatrixOS::SYS::Millis();
}

void pika_platform_reboot(void) {
  // Reboot the system
  MatrixOS::SYS::Reboot();
}

FILE* pika_platform_fopen(const char* filename, const char* modes) {

  FRESULT res;

  /* Check for valid initial mode character */
  if (!strchr("rwa", *modes))
  {
    return nullptr;
  }

  // Translate path for sandboxing
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(filename));
  if (translatedPath.empty())
  {
    return nullptr; // Access denied
  }

  // Ensure app directory exists for write operations
  if (strchr(modes, 'w') || strchr(modes, 'a') || strchr(modes, '+'))
  {
    MatrixOS::FileSystem::EnsureAppDirectory();
  }

  /* Compute the flags to pass to open() */
  int flags = 0;
  if (strchr(modes, '+'))
  {
    flags = FA_WRITE | FA_READ;
  }

  if (*modes == 'r')
  {
    if (!(flags & FA_WRITE))
    {
      flags |= FA_READ;
    }
  }
  else if (*modes == 'w')
  {
    flags |= FA_CREATE_ALWAYS | FA_WRITE;
  }
  else if (*modes == 'a')
  {
    flags |= FA_OPEN_APPEND | FA_WRITE;
  }

  FIL* fatFile = (FIL*)pika_platform_malloc(sizeof(FIL));
  if (nullptr == fatFile)
  {
    return nullptr;
  }
  res = f_open(fatFile, translatedPath.c_str(), flags);
  if (res)
  {
    pika_platform_free(fatFile);
    return nullptr;
  }

  return (FILE*)fatFile;
}

size_t pika_platform_fwrite(const void* ptr, size_t size, size_t n, FILE* stream) {
  FIL* fatFile = (FIL*)stream;
  size_t len = 0;
  FRESULT res = f_write(fatFile, ptr, n * size, &len);
  return len;
}

size_t pika_platform_fread(void* ptr, size_t size, size_t n, FILE* stream) {
  FIL* fatFile = (FIL*)stream;
  size_t len = 0;
  FRESULT res = f_read(fatFile, ptr, n * size, &len);
  return len;
}

int pika_platform_fclose(FILE* stream) {
  FIL* fatFile = (FIL*)stream;
  FRESULT res = f_close(fatFile);
  pika_platform_free(fatFile);
  return 0;
}

int pika_platform_fseek(FILE* stream, long offset, int whence) {
  FIL* fatFile = (FIL*)stream;
  DWORD fatfsOffset;
  switch (whence)
  {
  case SEEK_SET:
    fatfsOffset = offset;
    break;
  case SEEK_CUR:
    fatfsOffset = f_tell(fatFile) + offset;
    break;
  case SEEK_END:
    fatfsOffset = f_size(fatFile) + offset;
    break;
  default:
    return -1; // Invalid argument
  }
  FRESULT res = f_lseek(fatFile, fatfsOffset);
  int result = (res == FR_OK) ? 0 : -1;
  return result;
}

long pika_platform_ftell(FILE* stream) {
  FIL* fatFile = (FIL*)stream;
  long result = f_tell(fatFile);
  return result;
}

char* pika_platform_getcwd(char* buf, size_t size) {
  // FatFS doesn't directly provide a getcwd function. You might need
  // to manage the current directory yourself or return a default if it's
  // not crucial for your application.
  strncpy(buf, "/", size);
  return buf;
}

int pika_platform_chdir(const char* path) {
  // FatFS doesn't directly provide a chdir function. You might need
  // to manage the current directory yourself.
  return -1; // Not implemented
}

int pika_platform_rmdir(const char* pathname) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(pathname));
  if (translatedPath.empty())
  {
    return -1; // Access denied
  }
  FRESULT res = f_unlink(translatedPath.c_str());
  int result = (res == FR_OK) ? 0 : -1;
  return result;
}

int pika_platform_mkdir(const char* pathname, int mode) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(pathname));
  if (translatedPath.empty())
  {
    return -1; // Access denied
  }

  MatrixOS::FileSystem::EnsureAppDirectory();
  FRESULT res = f_mkdir(translatedPath.c_str());
  int result = (res == FR_OK) ? 0 : -1;
  return result;
}

int pika_platform_remove(const char* pathname) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(pathname));
  if (translatedPath.empty())
  {
    return -1; // Access denied
  }
  FRESULT res = f_unlink(translatedPath.c_str());
  int result = (res == FR_OK) ? 0 : -1;
  return result;
}

int pika_platform_rename(const char* oldpath, const char* newpath) {
  string oldTranslated = MatrixOS::FileSystem::TranslatePath(string(oldpath));
  string newTranslated = MatrixOS::FileSystem::TranslatePath(string(newpath));
  if (oldTranslated.empty() || newTranslated.empty())
  {
    return -1; // Access denied
  }
  FRESULT res = f_rename(oldTranslated.c_str(), newTranslated.c_str());
  int result = (res == FR_OK) ? 0 : -1;
  return result;
}

char** pika_platform_listdir(const char* path, int* count) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(path));
  if (translatedPath.empty())
  {
    *count = 0;
    return nullptr; // Access denied
  }

  DIR dir;
  FILINFO fno;
  FRESULT res;
  char** filelist = nullptr;
  int idx = 0, capacity = 10; // start with a capacity of 10, then grow if necessary

  res = f_opendir(&dir, translatedPath.c_str()); // Open the directory
  if (res == FR_OK)
  {
    filelist = (char**)pika_platform_malloc(capacity * sizeof(char*));
    if (!filelist)
      return nullptr; // Failed to allocate memory

    for (;;)
    {
      res = f_readdir(&dir, &fno); // Read a directory item
      if (res != FR_OK || fno.fname[0] == 0)
        break; // Break on error or end of dir

      if (idx == capacity)
      { // Grow the list if necessary
        capacity *= 2;
        char** newlist = (char**)pika_platform_realloc(filelist, capacity * sizeof(char*));
        if (!newlist)
        {
          // Free any previously allocated memory
          for (int i = 0; i < idx; i++)
            pika_platform_free(filelist[i]);
          pika_platform_free(filelist);
          *count = 0;
          return nullptr; // Failed to allocate more memory
        }
        filelist = newlist;
      }

      filelist[idx] = pika_platform_strdup(fno.fname);
      if (!filelist[idx])
      {
        // Free any previously allocated memory
        for (int i = 0; i < idx; i++)
          pika_platform_free(filelist[i]);
        pika_platform_free(filelist);
        *count = 0;
        return nullptr; // Failed to allocate memory for filename
      }
      idx++;
    }
    f_closedir(&dir);
  }

  *count = idx;
  return filelist;
}

int pika_platform_path_exists(const char* path) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(path));
  if (translatedPath.empty())
  {
    return 0; // Access denied
  }

  FRESULT res;
  FILINFO fno;

  res = f_stat(translatedPath.c_str(), &fno);
  int result = (res == FR_OK) ? 1 : 0;
  return result;
}

int pika_platform_path_isdir(const char* path) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(path));
  if (translatedPath.empty())
  {
    return 0; // Access denied
  }

  int isDir = 0;
  FRESULT res;
  FILINFO fno;

  res = f_stat(translatedPath.c_str(), &fno);
  if (res == FR_OK)
  {
    isDir = (fno.fattrib & AM_DIR) ? 1 : 0;
  }
  return isDir;
}

int pika_platform_path_isfile(const char* path) {
  string translatedPath = MatrixOS::FileSystem::TranslatePath(string(path));
  if (translatedPath.empty())
  {
    return 0; // Access denied
  }

  int isFile = 0;
  FRESULT res;
  FILINFO fno;

  res = f_stat(translatedPath.c_str(), &fno);
  if (res == FR_OK)
  {
    isFile = (fno.fattrib & AM_DIR) == 0 ? 1 : 0;
  }
  return isFile;
}
}

void Python::Setup(const vector<string>& args) {
  // Flush serial RX buffer
  while (MatrixOS::USB::CDC::Available())
  {
    (void)MatrixOS::USB::CDC::Read();
  }

  pikaMain = pikaPythonInit();

  // Check if a script path was provided
  if (!args.empty())
  {
    const string& pythonFilePath = args[0];
    MLOGD("Python", "Executing Python script: %s", pythonFilePath.c_str());

    // Execute the Python script file
    if (ExecutePythonFile(pythonFilePath))
    {
      MLOGD("Python", "Python script executed successfully");
    }
    else
    {
      MLOGE("Python", "Failed to execute Python script: %s", pythonFilePath.c_str());
    }
  }
  else
  {
    // No file specified, start REPL mode
    // Print Matrix OS ASCII art banner
    MatrixOS::USB::CDC::Println("");
    MatrixOS::USB::CDC::Println("███╗   ███╗ █████╗ ████████╗██████╗ ██╗██╗  ██╗     ██████╗ ███████╗");
    MatrixOS::USB::CDC::Println("████╗ ████║██╔══██╗╚══██╔══╝██╔══██╗██║╚██╗██╔╝    ██╔═══██╗██╔════╝");
    MatrixOS::USB::CDC::Println("██╔████╔██║███████║   ██║   ██████╔╝██║ ╚███╔╝     ██║   ██║███████╗");
    MatrixOS::USB::CDC::Println("██║╚██╔╝██║██╔══██║   ██║   ██╔══██╗██║ ██╔██╗     ██║   ██║╚════██║");
    MatrixOS::USB::CDC::Println("██║ ╚═╝ ██║██║  ██║   ██║   ██║  ██║██║██╔╝ ██╗    ╚██████╔╝███████║");
    MatrixOS::USB::CDC::Println("╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝");
    MatrixOS::USB::CDC::Println("Matrix OS Python REPL");
    MatrixOS::USB::CDC::Println("OS Version: " + MATRIXOS_VERSION_STRING);
    MatrixOS::USB::CDC::Println("See matrix.203.io for docs and usage guide.");
    MatrixOS::USB::CDC::Println("");

    pikaPythonShell(pikaMain);
  }

  Exit();
}

bool Python::ExecutePythonFile(const string& filePath) {
#if DEVICE_STORAGE == 1
  size_t lastSlash = filePath.find_last_of('/');
  if (lastSlash == string::npos)
  {
    MLOGD("Python", "No directory separator found in path: %s", filePath.c_str());
    return false;
  }

  // If the linked file path is already a py.a file.
  if (filePath.size() >= 5 && filePath.substr(filePath.size() - 5) == ".py.a")
  {
    // Link the library file and run "main" module
    MLOGD("Python", "Direct .py.a file execution: %s", filePath.c_str());
    obj_linkLibraryFile(pikaMain, (char*)filePath.c_str());
    obj_runModule(pikaMain, (char*)"main");
    return true;
  }

  // .py script
  string scriptDir = filePath.substr(0, lastSlash + 1); // Include trailing slash
  string apiDir = scriptDir + "pikascript-api";

  // Check if directory exists, create if not
  if (!MatrixOS::FileSystem::Exists(apiDir))
  {
    if (!MatrixOS::FileSystem::MakeDir(apiDir))
    {
      MLOGD("Python", "Failed to create pikascript-api directory: %s", apiDir.c_str());
      return false;
    }
  }

  // Check if compiled byte code
  string libFile = apiDir + "/pikaModules_cache.py.a";
  if (MatrixOS::FileSystem::Exists(libFile))
  {
    // Extract just the filename without extension
    string filename = filePath.substr(lastSlash + 1);
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == string::npos)
    {
      MLOGD("Python", "No file extension found in filename: %s", filename.c_str());
      return false;
    }

    string filenameNoExt = filename.substr(0, dotPos);

    // Link the library and run the module
    MLOGD("Python", "Found compiled library: %s", libFile.c_str());
    MLOGD("Python", "Running module: %s", filenameNoExt.c_str());
    obj_linkLibraryFile(pikaMain, (char*)libFile.c_str());
    obj_runModule(pikaMain, (char*)filenameNoExt.c_str());
  }
  else
  {
    // Run the file normally if no compiled library exists
    MLOGD("Python", "No compiled library found, running script directly: %s", filePath.c_str());
    pikaVM_runFile(pikaMain, (char*)filePath.c_str());
  }

  return true;
#else
  MLOGE("Python", "Filesystem not available, cannot execute Python file");
  return false;
#endif
}

void Python::End() {
  // Deinitialize PikaPython after shell exits
  obj_deinit(pikaMain);
}