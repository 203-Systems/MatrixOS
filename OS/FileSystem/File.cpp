#include "File.h"

#include "FatFS/ff.h"

#include <utility>

struct File::Impl {
  FIL fileHandle;
};

// File class implementation
File::File() : impl_(new Impl()), isOpen(false), filePath("") {}

File::File(File&& other) noexcept : impl_(other.impl_), filePath(std::move(other.filePath)), isOpen(other.isOpen) {
  other.impl_ = nullptr;
  other.filePath = "";
  other.isOpen = false;
}

File& File::operator=(File&& other) noexcept {
  if (this != &other)
  {
    if (isOpen)
    {
      Close();
    }

    delete impl_;
    impl_ = other.impl_;
    filePath = std::move(other.filePath);
    isOpen = other.isOpen;

    other.impl_ = nullptr;
    other.filePath = "";
    other.isOpen = false;
  }

  return *this;
}

File::~File() {
  if (isOpen)
  {
    Close();
  }

  delete impl_;
}

string File::Name() {
  return filePath;
}

bool File::Available() {
  if (!impl_ || !isOpen)
    return false;
  return !f_eof(&impl_->fileHandle);
}

bool File::Close() {
  if (!impl_ || !isOpen)
    return false;

  FRESULT result = f_close(&impl_->fileHandle);
  isOpen = false;
  filePath = "";

  return (result == FR_OK);
}

bool File::Flush() {
  if (!impl_ || !isOpen)
    return false;

  FRESULT result = f_sync(&impl_->fileHandle);
  return (result == FR_OK);
}

int File::Peek() {
  if (!impl_ || !isOpen)
    return -1;

  // Save current position
  FSIZE_t currentPos = f_tell(&impl_->fileHandle);

  // Read one byte
  UINT bytesRead;
  unsigned char byte;
  FRESULT result = f_read(&impl_->fileHandle, &byte, 1, &bytesRead);

  // Restore position
  f_lseek(&impl_->fileHandle, currentPos);

  if (result == FR_OK && bytesRead == 1)
  {
    return byte;
  }
  return -1;
}

size_t File::Position() {
  if (!impl_ || !isOpen)
    return 0;
  return f_tell(&impl_->fileHandle);
}

void File::Print(const string& data) {
  if (!impl_ || !isOpen)
    return;

  UINT bytesWritten;
  f_write(&impl_->fileHandle, data.c_str(), data.length(), &bytesWritten);
}

void File::Println(const string& data) {
  Print(data + "\\n");
}

bool File::Seek(size_t position, SeekMode whence) {
  if (!impl_ || !isOpen)
    return false;

  FSIZE_t targetPos = position;

  if (whence == FROM_CURRENT)
  {
    targetPos = f_tell(&impl_->fileHandle) + position;
  }
  else if (whence == FROM_END)
  {
    targetPos = f_size(&impl_->fileHandle) + position;
  }

  FRESULT result = f_lseek(&impl_->fileHandle, targetPos);
  return (result == FR_OK);
}

size_t File::Size() {
  if (!impl_ || !isOpen)
    return 0;
  return f_size(&impl_->fileHandle);
}

size_t File::Read(void* buffer, size_t length) {
  if (!impl_ || !isOpen || !buffer)
    return 0;

  UINT bytesRead;
  FRESULT result = f_read(&impl_->fileHandle, buffer, length, &bytesRead);

  if (result == FR_OK)
  {
    return bytesRead;
  }
  return 0;
}

size_t File::Write(const void* buffer, size_t length) {
  if (!impl_ || !isOpen || !buffer)
    return 0;

  UINT bytesWritten;
  FRESULT result = f_write(&impl_->fileHandle, buffer, length, &bytesWritten);

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
  if (!impl_)
  {
    impl_ = new Impl();
  }

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

  FRESULT result = f_open(&impl_->fileHandle, path.c_str(), accessMode);

  if (result == FR_OK)
  {
    isOpen = true;
    filePath = path;
    return true;
  }

  return false;
}
