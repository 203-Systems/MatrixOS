#pragma once

#include "MatrixOS.h"

namespace MatrixOS::Command
{
enum class Encoding : uint8_t { HID, SysEx7Bit };

using ReplyCallback = bool (*)(const vector<uint8_t>& reply, bool end, void* context);

bool Handle(const uint8_t* request, size_t size, Encoding encoding, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext);
} // namespace MatrixOS::Command
