// FileSystem stub for builds without storage (DEVICE_STORAGE == 0)
// Provides compile-compatible but non-functional File and FileSystem implementations.

#include "MatrixOS.h"

#include <utility>

struct File::Impl {};

// --- File class stubs ---
File::File() : impl_(new Impl()), isOpen(false) {}
File::File(File&& other) noexcept : impl_(other.impl_), filePath(std::move(other.filePath)), isOpen(other.isOpen) {
  other.impl_ = nullptr;
  other.filePath = "";
  other.isOpen = false;
}

File& File::operator=(File&& other) noexcept {
  if (this != &other)
  {
    Close();
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
  Close();
  delete impl_;
}

string File::Name() { return filePath; }
bool File::Available() { return false; }
bool File::Close() { isOpen = false; return true; }
bool File::Flush() { return false; }
int File::Peek() { return -1; }
size_t File::Position() { return 0; }
void File::Print(const string& data) {}
void File::Println(const string& data) {}
bool File::Seek(size_t position, SeekMode whence) { return false; }
size_t File::Size() { return 0; }
size_t File::Read(void* buffer, size_t length) { return 0; }
size_t File::Write(const void* buffer, size_t length) { return 0; }
bool File::IsDirectory() { return false; }
bool File::_Open(const string& path, const string& mode) { return false; }

// --- FileSystem namespace stubs ---
namespace MatrixOS::FileSystem
{
  void Init() {}
  bool Available() { return false; }
  bool Exists(const string& path) { return false; }
  bool MakeDir(const string& path) { return false; }
  File Open(const string& path, const string& mode) { return File(); }
  bool Remove(const string& path) { return false; }
  bool RemoveDir(const string& path) { return false; }
  bool Rename(const string& from, const string& to) { return false; }
  vector<string> ListDir(const string& path) { return {}; }
}
