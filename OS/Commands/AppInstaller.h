#pragma once
#include "CommandHandler.h"

namespace MatrixOS::AppInstaller
{
// All install I/O runs serially on the existing system command task.
bool Handle(const uint8_t* request, size_t size, Command::ReplyCallback callback);
void Poll();
// Developer APP releases its install session through the normal End() lifecycle.
void Cancel();
}
