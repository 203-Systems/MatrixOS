#pragma once

#include "Framework.h"

enum SeekMode : int {
	FROM_START = 0,
	FROM_CURRENT = 1,
	FROM_END = 2
};

class File {
private:
	struct Impl;

	Impl* impl_ = nullptr;
	string filePath;
	bool isOpen = false;

public:
	File();
	~File();

	File(const File&) = delete;
	File& operator=(const File&) = delete;
	File(File&& other) noexcept;
	File& operator=(File&& other) noexcept;

	string Name();
	bool Available();
	bool Close();
	bool Flush();
	int Peek();
	size_t Position();
	void Print(const string& data);
	void Println(const string& data);
	bool Seek(size_t position, SeekMode whence = FROM_START);
	size_t Size();
	size_t Read(void* buffer, size_t length);
	size_t Write(const void* buffer, size_t length);
	bool IsDirectory();

	// Internal method used by MatrixOS::FileSystem::Open.
	bool _Open(const string& path, const string& mode);
};