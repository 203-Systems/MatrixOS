#pragma once

#include "MatrixOS.h"

namespace MatrixOS::Command
{
enum class Encoding : uint8_t { HID, SysEx7Bit };

using ReplyCallback = bool (*)(const vector<uint8_t>& reply, bool end, void* context);

void Init();
bool Submit(Encoding encoding, const uint8_t* request, size_t size, size_t maxReplyLength, ReplyCallback replyCallback, uint16_t replyPort = 0);
bool Handle(const uint8_t* request, size_t size, Encoding encoding, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext);
} // namespace MatrixOS::Command
