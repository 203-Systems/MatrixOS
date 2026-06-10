#pragma once

#include "NotePad.h"
#include <stdint.h>

enum class NoteConfigNVSLoadResult : uint8_t {
  Current,
  Upgraded,
  DefaultsSaved,
};

NoteConfigNVSLoadResult LoadOrUpgradeNoteConfigs(NotePadConfig notePadConfigs[2], uint32_t nvsVersion);
void SaveNoteConfigsToNVS(NotePadConfig notePadConfigs[2]);
