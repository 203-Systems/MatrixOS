#include "File.h"

// File class implementation
File::File() : is_open(false), file_path("") {
}

File::~File() {
  if (is_open) {
    Close();
  }
}

string File::Name() {
  return file_path;
}

bool File::Available() {
  if (!is_open) return false;
  return !f_eof(&file_handle);
}

bool File::Close() {
  if (!is_open) return false;

  FRESULT result = f_close(&file_handle);
  is_open = false;
  file_path = "";

  return (result == FR_OK);
}

bool File::Flush() {
  if (!is_open) return false;

  FRESULT result = f_sync(&file_handle);
  return (result == FR_OK);
}

int File::Peek() {
  if (!is_open) return -1;

  // Save current position
  FSIZE_t current_pos = f_tell(&file_handle);

  // Read one byte
  UINT bytes_read;
  unsigned char byte;
  FRESULT result = f_read(&file_handle, &byte, 1, &bytes_read);

  // Restore position
  f_lseek(&file_handle, current_pos);

  if (result == FR_OK && bytes_read == 1) {
    return byte;
  }
  return -1;
}

size_t File::Position() {
  if (!is_open) return 0;
  return f_tell(&file_handle);
}

void File::Print(const string& data) {
  if (!is_open) return;

  UINT bytes_written;
  f_write(&file_handle, data.c_str(), data.length(), &bytes_written);
}

void File::Println(const string& data) {
  Print(data + "\\n");
}

bool File::Seek(size_t position, SeekMode whence) {
  if (!is_open) return false;

  FSIZE_t target_pos = position;

  if (whence == FROM_CURRENT) {
    target_pos = f_tell(&file_handle) + position;
  } else if (whence == FROM_END) {
    target_pos = f_size(&file_handle) + position;
  }

  FRESULT result = f_lseek(&file_handle, target_pos);
  return (result == FR_OK);
}

size_t File::Size() {
  if (!is_open) return 0;
  return f_size(&file_handle);
}

size_t File::Read(void* buffer, size_t length) {
  if (!is_open || !buffer) return 0;

  UINT bytes_read;
  FRESULT result = f_read(&file_handle, buffer, length, &bytes_read);

  if (result == FR_OK) {
    return bytes_read;
  }
  return 0;
}

size_t File::Write(const void* buffer, size_t length) {
  if (!is_open || !buffer) return 0;

  UINT bytes_written;
  FRESULT result = f_write(&file_handle, buffer, length, &bytes_written);

  if (result == FR_OK) {
    return bytes_written;
  }
  return 0;
}

bool File::IsDirectory() {
  // For an open file, check if it was opened as a directory
  // This is a simplification - actual files can't be directories
  return false;
}

bool File::_Open(const string& path, const string& mode) {
  if (is_open) {
    Close();
  }

  // Convert mode string to FatFS access mode
  BYTE access_mode = 0;

  if (mode.find('r') != string::npos) {
    access_mode |= FA_READ;
  }
  if (mode.find('w') != string::npos) {
    access_mode |= FA_WRITE | FA_CREATE_ALWAYS;
  }
  if (mode.find('a') != string::npos) {
    access_mode |= FA_WRITE | FA_OPEN_APPEND;
  }
  if (mode.find('+') != string::npos) {
    access_mode |= FA_READ | FA_WRITE;
  }

  // Default to read if no mode specified
  if (access_mode == 0) {
    access_mode = FA_READ;
  }

  FRESULT result = f_open(&file_handle, path.c_str(), access_mode);

  if (result == FR_OK) {
    is_open = true;
    file_path = path;
    return true;
  }

  return false;
}