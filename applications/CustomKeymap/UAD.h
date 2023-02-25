#pragma once

#include "MatrixOS.h"
#include "cb0r.h"
#include "Action.h"

#define UAD_VERSION 0

// Universal Action Descriptor
class UAD
{
  public:
  UAD();
  UAD(uint8_t* uad, size_t size);
  ~UAD();

  bool LoadUAD(uint8_t* uad, size_t size);
  void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);

  // UAD Runtime
  bool ExecuteActions(ActionInfo* actionInfo, KeyInfo* keyInfo); //WIll pick a layer and index for ExeciteAction
  bool ExecuteAction(ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo); //Not intented for direct use
  // bool GetActionsFromOffset(uint16_t offset, cb0r_t result);

  // Action API
  bool SetRegister(ActionInfo actionInfo, uint32_t value);
  bool GetRegister(ActionInfo actionInfo, uint32_t* value);
  bool ClearRegister(ActionInfo actionInfo);
  void SetLayer(uint8_t layer, bool state);
  void UnsetLayer(uint8_t layer);

  // Helpers
  int8_t IndexInBitmap(uint64_t bitmap, uint8_t index); // Not this one has +1 offset (Because usually used in array index look up)
  uint8_t GetTopLayer();

 private:
  uint8_t* uad;
  bool loaded = false;
  size_t uadSize;
  Dimension mapSize;
  uint8_t layerCount;
  vector<uint32_t> actionList;
  vector<uint32_t> effectList;
  uint16_t layerEnabled = 1;
  uint16_t layerPassthough = 0xFFFF;

  uint16_t** actionLUT;
  uint16_t** effectLUT;

  std::unordered_map<ActionInfo, uint32_t, ActionInfoHash> registers;

  // UAD Loader
  bool CheckVersion(cb0r_t uadMap);
  bool LoadActionList(cb0r_t uadMap);
  bool LoadEffectList(cb0r_t uadMap);
  bool CreateHashList(cb0r_t cborArray, vector<uint32_t>* list); // Used to generate hash for action names
  bool CreateLUT(cb0r_t actionMatrix, uint16_t*** lut, Dimension lutSize);
  bool LoadDevice(cb0r_t uadMap);
};

#define IsBitSet(byte, bit) ((byte & (1 << bit)) != 0)