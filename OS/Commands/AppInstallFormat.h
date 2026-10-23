#pragma once
#include <cstdint>
#include <string>
#include <cctype>
#include <algorithm>

namespace MatrixOS::AppInstallFormat
{
constexpr uint32_t MaxPackage = 8 * 1024 * 1024;
constexpr uint32_t MaxManifest = 32768;
constexpr uint16_t MaxFiles = 128;
inline uint32_t U32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
inline uint32_t CRC(uint32_t crc, const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1)));
  }
  return crc;
}
inline bool AppName(const std::string& s) {
  if (s.empty() || s.size() > 64) return false;
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char c = s[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (i && (c == '_' || c == '-')))) return false;
  }
  return true;
}
inline std::string Fold(std::string s) {
  for (auto& c : s) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return s;
}
inline bool Path(const std::string& s) {
  // ASCII names avoid FAT/OEM Unicode alias ambiguity in this first version.
  if (s.empty() || s.size() > 160) return false;
  if (std::count(s.begin(), s.end(), '/') > 15) return false;
  size_t start = 0;
  while (start < s.size()) {
    size_t end = s.find('/', start);
    if (end == std::string::npos) end = s.size();
    std::string part = s.substr(start, end - start);
    if (part.empty() || part == "." || part == ".." || part.back() == ' ' || part.back() == '.') return false;
    for (unsigned char c : part) if (c < 32 || c >= 127 || std::string("\\:*?\"<>|").find(c) != std::string::npos) return false;
    std::string base = Fold(part.substr(0, part.find('.')));
    if (base == "con" || base == "prn" || base == "aux" || base == "nul" ||
        (base.size() == 4 && (base.substr(0, 3) == "com" || base.substr(0, 3) == "lpt") && base[3] >= '1' && base[3] <= '9')) return false;
    if (end == s.size()) return true;
    start = end + 1;
  }
  return false;
}
}
