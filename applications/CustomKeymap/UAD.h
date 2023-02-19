#pragma once

#include "MatrixOS.h"
#include "cb0r.h"

#define UAD_VERSION 0

// Universal Action Descriptor
class UAD
{
  public:
  UAD(uint8_t* uad, size_t size);
  ~UAD();

  void UpdateEffects();
  void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);

 private:
  uint8_t* uad;
  size_t uadSize;
  Dimension mapSize;
  uint8_t layerCount;
  vector<uint32_t> actionList;
  vector<uint32_t> effectList;
  uint8_t currentLayer = 0;
  uint16_t** actionLUT;
  uint16_t** effectLUT;


  // UAD Loading
  bool LoadUAD(uint8_t* uad, size_t size);
  bool CheckVersion(cb0r_t uadMap);
  bool LoadActionList(cb0r_t uadMap);
  bool LoadEffectList(cb0r_t uadMap);
  bool CreateHashList(cb0r_t cbor_array, vector<uint32_t>* list);
  bool CreateLUT(cb0r_t actionMatrix, uint16_t*** lut, Dimension lutSize);
  bool LoadDevice(cb0r_t uadMap);

  // UAD Runtime
  bool ExecuteActionsFromOffset(uint16_t offset, uint8_t layer, uint16_t keyID, KeyInfo* keyInfo);
  bool ExecuteAction(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo);
  bool UpdateEffectsFromOffset(uint16_t offset, uint8_t layer, uint16_t keyID, KeyInfo* keyInfo);
  bool UpdateEffect(cb0r_t effect, uint16_t keyID, KeyInfo* keyInfo);
  bool GetActionFromOffset(uint16_t offset, uint16_t layer, cb0r_t result);

  // Helpers
  uint8_t IndexInBitmap(uint64_t bitmap, uint8_t index);
};