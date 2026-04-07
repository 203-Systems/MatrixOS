#pragma once

#include <stdint.h>

#ifndef MPE_COPROCESSOR_LINK_PROTOCOL_FETCH_MAX_ENTRIES
#define MPE_COPROCESSOR_LINK_PROTOCOL_FETCH_MAX_ENTRIES 8U
#endif

enum {
  MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_SET_THRESHOLDS = 0x00,
  MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_GET_THRESHOLDS = 0x01,
  MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_CACHE_CHANGES = 0x02,
  MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_FETCH_CHANGES = 0x03,
};

typedef struct __attribute__((packed)) {
  uint16_t lowThreshold;
  uint16_t changeThreshold;
  uint16_t highThreshold;
} mpe_coprocessor_link_protocol_thresholds_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  mpe_coprocessor_link_protocol_thresholds_t thresholds;
} mpe_coprocessor_link_protocol_get_thresholds_rsp_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t cachedCount;
} mpe_coprocessor_link_protocol_cache_changes_rsp_t;

typedef struct __attribute__((packed)) {
  uint8_t cursor;
  uint8_t maxCount;
} mpe_coprocessor_link_protocol_fetch_changes_req_t;

typedef struct __attribute__((packed)) {
  uint8_t x;
  uint8_t y;
  uint16_t reading[8];
} mpe_coprocessor_link_protocol_change_entry_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t returnedCount;
  mpe_coprocessor_link_protocol_change_entry_t entries[MPE_COPROCESSOR_LINK_PROTOCOL_FETCH_MAX_ENTRIES];
} mpe_coprocessor_link_protocol_fetch_changes_rsp_t;
