#include "File.h"

// File class implementation
File::File() : isOpen(false), filePath("") {}

File::~File() {
  if (isOpen)
  {
    Close();
  }
}

string File::Name() {
  return filePath;
}

bool File::Available() {
  if (!isOpen)
    return false;
  return !f_eof(&fileHandle);
}

bool File::Close() {
  if (!isOpen)
    return false;

  FRESULT result = f_close(&fileHandle);
  isOpen = false;
  filePath = "";

  return (result == FR_OK);
}

bool File::Flush() {
  if (!isOpen)
    return false;

  FRESULT result = f_sync(&fileHandle);
  return (result == FR_OK);
}

int File::Peek() {
  if (!isOpen)
    return -1;

  // Save current position
  FSIZE_t currentPos = f_tell(&fileHandle);

  // Read one byte
  UINT bytesRead;
  unsigned char byte;
  FRESULT result = f_read(&fileHandle, &byte, 1, &bytesRead);

  // Restore position
  f_lseek(&fileHandle, currentPos);

  if (result == FR_OK && bytesRead == 1)
  {
    return byte;
  }
  return -1;
}

size_t File::Position() {
  if (!isOpen)
    return 0;
  return f_tell(&fileHandle);
}

void File::Print(const string& data) {
  if (!isOpen)
    return;

  UINT bytesWritten;
  f_write(&fileHandle, data.c_str(), data.length(), &bytesWritten);
}

void File::Println(const string& data) {
  Print(data + "\\n");
}

bool File::Seek(size_t position, SeekMode whence) {
  if (!isOpen)
    return false;

  FSIZE_t targetPos = position;

  if (whence == FROM_CURRENT)
  {
    targetPos = f_tell(&fileHandle) + position;
  }
  else if (whence == FROM_END)
  {
    targetPos = f_size(&fileHandle) + position;
  }

  FRESULT result = f_lseek(&fileHandle, targetPos);
  return (result == FR_OK);
}

size_t File::Size() {
  if (!isOpen)
    return 0;
  return f_size(&fileHandle);
}

size_t File::Read(void* buffer, size_t length) {
  if (!isOpen || !buffer)
    return 0;

  UINT bytesRead;
  FRESULT result = f_read(&fileHandle, buffer, length, &bytesRead);

  if (result == FR_OK)
  {
    return bytesRead;
  }
  return 0;
}

size_t File::Write(const void* buffer, size_t length) {
  if (!isOpen || !buffer)
    return 0;

  UINT bytesWritten;
  FRESULT result = f_write(&fileHandle, buffer, length, &bytesWritten);

  if (result == FR_OK)
  {
    return bytesWritten;
  }
  return 0;
}

bool File::IsDirectory() {
  // For an open file, check if it was opened as a directory
  // This is a simplification - actual files can't be directories
  return false;
}

bool File::_Open(const string& path, const string& mode) {
  if (isOpen)
  {
    Close();
  }

  // Convert mode string to FatFS access mode
  BYTE accessMode = 0;

  if (mode.find('r') != string::npos)
  {
    accessMode |= FA_READ;
  }
  if (mode.find('w') != string::npos)
  {
    accessMode |= FA_WRITE | FA_CREATE_ALWAYS;
  }
  if (mode.find('a') != string::npos)
  {
    accessMode |= FA_WRITE | FA_OPEN_APPEND;
  }
  if (mode.find('+') != string::npos)
  {
    accessMode |= FA_READ | FA_WRITE;
  }

  // Default to read if no mode specified
  if (accessMode == 0)
  {
    accessMode = FA_READ;
  }

  FRESULT result = f_open(&fileHandle, path.c_str(), accessMode);

  if (result == FR_OK)
  {
    isOpen = true;
    filePath = path;
    return true;
  }

  return false;
}
