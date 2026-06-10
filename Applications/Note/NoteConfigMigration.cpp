#include "NoteConfigMigration.h"
#include "Note.h"
#include <cstring>

namespace
{
struct NotePadConfigV2 {
  uint8_t rootKey = 0;
  uint8_t rootOffset = 0;
  uint16_t scale = MINOR;
  int8_t octave = 2;
  uint8_t channel = 0;
  NoteLayoutMode mode = OCTAVE_LAYOUT;
  bool enforceScale = true;
  union {
    struct
    {
      uint8_t unknown = 0;
    };
    struct
    {
      uint8_t xOffset : 4;
      uint8_t y_offset : 4;
    };
    struct
    {
      uint8_t unknown2;
    };
  };
  bool forceSensitive = true;
  Color rootColor = Color(0x0040FF);
  Color color = Color(0x00FFFF);
  ColorMode colorMode = ROOT_N_SCALE;
  bool useWhiteAsOutOfScale = false;
  ArpeggiatorConfig arpConfig;
};

void MigrateNotePadConfigV2(const NotePadConfigV2& source, NotePadConfig& target) {
  std::memcpy(&target, &source, sizeof(source));
  target.defaultVelocity = 127;
}

bool SanitizeNoteConfigs(NotePadConfig notePadConfigs[2]) {
  bool modified = false;

  for (uint8_t i = 0; i < 2; i++)
  {
    if (notePadConfigs[i].defaultVelocity == 0 || notePadConfigs[i].defaultVelocity > 127)
    {
      notePadConfigs[i].defaultVelocity = 127;
      modified = true;
    }
  }

  return modified;
}
} // namespace

void SaveNoteConfigsToNVS(NotePadConfig notePadConfigs[2]) {
  MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(NotePadConfig) * 2);
}

NoteConfigNVSLoadResult LoadOrUpgradeNoteConfigs(NotePadConfig notePadConfigs[2], uint32_t nvsVersion) {
  size_t storedSize = MatrixOS::NVS::GetSize(NOTE_CONFIGS_HASH);

  if (nvsVersion == NOTE_APP_VERSION && storedSize == sizeof(NotePadConfig) * 2)
  {
    MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(NotePadConfig) * 2);
    if (SanitizeNoteConfigs(notePadConfigs))
    {
      SaveNoteConfigsToNVS(notePadConfigs);
    }
    return NoteConfigNVSLoadResult::Current;
  }

  if (nvsVersion == 2 && storedSize == sizeof(NotePadConfigV2) * 2)
  {
    NotePadConfigV2 legacyConfigs[2];
    MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, legacyConfigs, sizeof(legacyConfigs));
    for (uint8_t i = 0; i < 2; i++)
    {
      MigrateNotePadConfigV2(legacyConfigs[i], notePadConfigs[i]);
    }
    SaveNoteConfigsToNVS(notePadConfigs);
    return NoteConfigNVSLoadResult::Upgraded;
  }

  MLOGD("Note", "Config version or size mismatch: version=%d, stored=%d, expected=%d. Using defaults.", nvsVersion,
        (unsigned)storedSize, (unsigned)(sizeof(NotePadConfig) * 2));
  SaveNoteConfigsToNVS(notePadConfigs);
  return NoteConfigNVSLoadResult::DefaultsSaved;
}
