#pragma once

#include <stdint.h>

#ifndef MYSTRIX2_MPE_FETCH_MAX_ENTRIES
#define MYSTRIX2_MPE_FETCH_MAX_ENTRIES 8U
#endif

enum {
  MYSTRIX2_MPE_APP_CMD_SET_THRESHOLDS = 0x00,
  MYSTRIX2_MPE_APP_CMD_GET_THRESHOLDS = 0x01,
  MYSTRIX2_MPE_APP_CMD_CACHE_CHANGES = 0x02,
  MYSTRIX2_MPE_APP_CMD_FETCH_CHANGES = 0x03,
};

typedef struct __attribute__((packed)) {
  uint16_t lowThreshold;
  uint16_t changeThreshold;
  uint16_t highThreshold;
} mystrix2_mpe_thresholds_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  mystrix2_mpe_thresholds_t thresholds;
} mystrix2_mpe_get_thresholds_rsp_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t cachedCount;
} mystrix2_mpe_cache_changes_rsp_t;

typedef struct __attribute__((packed)) {
  uint8_t cursor;
  uint8_t maxCount;
} mystrix2_mpe_fetch_changes_req_t;

typedef struct __attribute__((packed)) {
  uint8_t x;
  uint8_t y;
  uint16_t reading[8];
} mystrix2_mpe_change_entry_t;

typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t returnedCount;
  mystrix2_mpe_change_entry_t entries[MYSTRIX2_MPE_FETCH_MAX_ENTRIES];
} mystrix2_mpe_fetch_changes_rsp_t;
