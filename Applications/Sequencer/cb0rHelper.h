#pragma once

#include "cb0r.h"
#include <vector>
#include <string>

inline void cb_write_uint(std::vector<uint8_t>& out, cb0r_e type, uint64_t value)
{
    uint8_t buf[9];
    uint8_t len = cb0r_write(buf, type, value);
    out.insert(out.end(), buf, buf + len);
}

inline void cb_write_bytes(std::vector<uint8_t>& out, const uint8_t* data, size_t len)
{
    cb_write_uint(out, CB0R_BYTE, len);
    out.insert(out.end(), data, data + len);
}

inline void cb_write_text(std::vector<uint8_t>& out, const std::string& s)
{
    cb_write_uint(out, CB0R_UTF8, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

inline void cb_write_bool(std::vector<uint8_t>& out, bool v)
{
    out.push_back(v ? 0xF5 : 0xF4); // CBOR true/false
}
