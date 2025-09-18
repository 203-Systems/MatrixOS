#include "Python.h"
#include "pikaScript.h"
#include "FileSystem/FatFS/ff.h"

// Forward declarations - these will be linked from the actual implementations
namespace MatrixOS::FileSystem {
  extern string TranslatePath(const string& path);
  extern void EnsureAppDirectory();
}


extern "C" {
  /* private prototypes */
  int __fmodeflags(const char* mode);

  FILE* pika_platform_fopen(const char* filename, const char* modes) {

    FRESULT res;
    int flags;

    /* Check for valid initial mode character */
    if (!strchr("rwa", * modes)) {
      return 0;
    }

    // Translate path for sandboxing
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(filename));
    if (translated_path.empty()) {
      return NULL; // Access denied
    }

    // Ensure app directory exists for write operations
    if (strchr(modes, 'w') || strchr(modes, 'a') || strchr(modes, '+')) {
      MatrixOS::FileSystem::EnsureAppDirectory();
    }

    /* Compute the flags to pass to open() */
    flags = __fmodeflags(modes);

    FIL* _f = (FIL*)pika_platform_malloc(sizeof(FIL));
    if (NULL == _f) {
      return NULL;
    }
    res = f_open(_f, translated_path.c_str(), flags);
    if (res) {
      pika_platform_free(_f);
      return NULL;
    }

    return (FILE*)_f;
  }

  size_t pika_platform_fwrite(const void* ptr, size_t size, size_t n, FILE* stream) {
    FIL* _f = (FIL*)stream;
    size_t len = 0;
    FRESULT res = f_write(_f, ptr, n * size, &len);
    return len;
  }

  size_t pika_platform_fread(void* ptr, size_t size, size_t n, FILE* stream) {
    FIL* _f = (FIL*)stream;
    size_t len = 0;
    FRESULT res = f_read(_f, ptr, n * size, &len);
    return len;
  }

  int pika_platform_fclose(FILE* stream) {
    FIL* _f = (FIL*)stream;
    FRESULT res = f_close(_f);
    pika_platform_free(_f);
    return 0;
  }

  int pika_platform_fseek(FILE* stream, long offset, int whence) {
    FIL* _f = (FIL*)stream;
    DWORD fatfs_offset;
    switch (whence) {
    case SEEK_SET:
      fatfs_offset = offset;
      break;
    case SEEK_CUR:
      fatfs_offset = f_tell(_f) + offset;
      break;
    case SEEK_END:
      fatfs_offset = f_size(_f) + offset;
      break;
    default:
      return -1; // Invalid argument
    }
    FRESULT res = f_lseek(_f, fatfs_offset);
    int result = (res == FR_OK) ? 0 : -1;
    return result;
  }

  long pika_platform_ftell(FILE* stream) {
    FIL* _f = (FIL*)stream;
    long result = f_tell(_f);
    return result;
  }

  int __fmodeflags(const char * mode) {
    int flags = 0;

    if (strchr(mode, '+')) {
      flags = FA_WRITE | FA_READ;
    }

    if ( * mode == 'r') {
      if (!(flags & FA_WRITE)) {
        flags |= FA_READ;
      }
    } else if ( * mode == 'w') {
      flags |= FA_CREATE_ALWAYS | FA_WRITE;
    } else if ( * mode == 'a') {
      flags |= FA_OPEN_APPEND | FA_WRITE;
    }

    return flags;
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
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(pathname));
    if (translated_path.empty()) {
      return -1; // Access denied
    }
    FRESULT res = f_unlink(translated_path.c_str());
    int result = (res == FR_OK) ? 0 : -1;
    return result;
  }

  int pika_platform_mkdir(const char* pathname, int mode) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(pathname));
    if (translated_path.empty()) {
      return -1; // Access denied
    }

    MatrixOS::FileSystem::EnsureAppDirectory();
    FRESULT res = f_mkdir(translated_path.c_str());
    int result = (res == FR_OK) ? 0 : -1;
    return result;
  }

  int pika_platform_remove(const char* pathname) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(pathname));
    if (translated_path.empty()) {
      return -1; // Access denied
    }
    FRESULT res = f_unlink(translated_path.c_str());
    int result = (res == FR_OK) ? 0 : -1;
    return result;
  }

  int pika_platform_rename(const char* oldpath, const char* newpath) {
    string old_translated = MatrixOS::FileSystem::TranslatePath(string(oldpath));
    string new_translated = MatrixOS::FileSystem::TranslatePath(string(newpath));
    if (old_translated.empty() || new_translated.empty()) {
      return -1; // Access denied
    }
    FRESULT res = f_rename(old_translated.c_str(), new_translated.c_str());
    int result = (res == FR_OK) ? 0 : -1;
    return result;
  }

  char** pika_platform_listdir(const char* path, int* count) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(path));
    if (translated_path.empty()) {
      *count = 0;
      return NULL; // Access denied
    }

    DIR dir;
    static FILINFO fno;
    FRESULT res;
    char ** filelist = NULL;
    int idx = 0, capacity = 10; // start with a capacity of 10, then grow if necessary

    res = f_opendir( & dir, translated_path.c_str()); // Open the directory
    if (res == FR_OK) {
      filelist = (char**)pika_platform_malloc(capacity * sizeof(char * ));
      if (!filelist)
        return NULL; // Failed to allocate memory

      for (;;) {
        res = f_readdir( & dir, & fno); // Read a directory item
        if (res != FR_OK || fno.fname[0] == 0)
          break; // Break on error or end of dir

        if (idx == capacity) { // Grow the list if necessary
          capacity *= 2;
          char ** newlist = (char**)pika_platform_realloc(filelist, capacity * sizeof(char * ));
          if (!newlist) {
            // Free any previously allocated memory
            for (int i = 0; i < idx; i++)
              pika_platform_free(filelist[i]);
            pika_platform_free(filelist);
            * count = 0;
            return NULL; // Failed to allocate more memory
          }
          filelist = newlist;
        }

        filelist[idx] = pika_platform_strdup(fno.fname);
        if (!filelist[idx]) {
          // Free any previously allocated memory
          for (int i = 0; i < idx; i++)
            pika_platform_free(filelist[i]);
          pika_platform_free(filelist);
          * count = 0;
          return NULL; // Failed to allocate memory for filename
        }
        idx++;
      }
      f_closedir( & dir);
    }

    *count = idx;
    return filelist;
  }

  int pika_platform_path_exists(const char* path) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(path));
    if (translated_path.empty()) {
      return 0; // Access denied
    }

    FRESULT res;
    FILINFO fno;

    res = f_stat(translated_path.c_str(), &fno);
    int result = (res == FR_OK) ? 1 : 0;
    return result;
  }

  int pika_platform_path_isdir(const char* path) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(path));
    if (translated_path.empty()) {
      return 0; // Access denied
    }

    int is_dir = 0;
    FRESULT res;
    FILINFO fno;

    res = f_stat(translated_path.c_str(), &fno);
    if (res == FR_OK) {
      is_dir = (fno.fattrib & AM_DIR) ? 1 : 0;
    }
    return is_dir;
  }

  int pika_platform_path_isfile(const char* path) {
    string translated_path = MatrixOS::FileSystem::TranslatePath(string(path));
    if (translated_path.empty()) {
      return 0; // Access denied
    }

    int is_file = 0;
    FRESULT res;
    FILINFO fno;

    res = f_stat(translated_path.c_str(), &fno);
    if (res == FR_OK) {
      is_file = (fno.fattrib & AM_DIR) == 0 ? 1 : 0;
    }
    return is_file;
  }
}