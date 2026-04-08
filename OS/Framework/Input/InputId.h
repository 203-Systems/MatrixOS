#pragma once

#include <stdint.h>
#include <type_traits>

struct InputId {
  uint8_t clusterId;
  uint16_t localIndex;

  static constexpr uint8_t invalidClusterId = UINT8_MAX;
  static constexpr uint16_t invalidLocalIndex = UINT16_MAX;

  static constexpr InputId Invalid() {
    return InputId{invalidClusterId, invalidLocalIndex};
  }

  // The system function key (maps to old FUNCTION_KEY = 0)
  static constexpr InputId FunctionKey() {
    return InputId{0, 0};
  }

  bool IsFunctionKey() const {
    return clusterId == 0 && localIndex == 0;
  }

  bool operator==(const InputId& target) const {
    return clusterId == target.clusterId && localIndex == target.localIndex;
  }

  bool operator!=(const InputId& target) const {
    return !(*this == target);
  }

  explicit operator bool() const {
    return clusterId != invalidClusterId;
  }
};

static_assert(std::is_trivially_copyable_v<InputId>, "InputId must be trivially copyable");
