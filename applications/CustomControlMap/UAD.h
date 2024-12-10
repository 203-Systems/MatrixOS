#pragma once

#include "MatrixOS.h"
#include "cb0r.h"
#include "cb0r_utils.h"
#include "Action.h"

#define UAD_VERSION 0

// Universal Action Descriptor
class UADRuntime
{
  public:
  static const uint8_t UAD_MAJOR_VERSION = 0;
  static const uint8_t UAD_MINOR_VERSION = 1;

  UADRuntime();
  UADRuntime(uint8_t* uad, size_t size);
  ~UADRuntime();

  bool LoadUAD(uint8_t* uad, size_t size);
  void UnloadUAD();
  void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);

  // UAD Runtime
  enum ActionEventType { INITIALIZATION, UPDATE, DEINITIALIZATION, KEYEVENT};
  struct ActionEvent
  {
    ActionEventType type;
    union
    {
      void* data; // For GENERIC EVENT TYPES
      KeyInfo* keyInfo; // For KEYEVENT
    };
  };

  bool ExecuteActions(ActionInfo* actionInfo, ActionEvent* actionEvent); //WIll pick a layer and index for Action
  bool ExecuteEffects(ActionInfo* effectInfo, ActionEvent* effectEvent); //WIll pick a layer and index for Effect
  bool ExecuteAction(ActionInfo* actionInfo, cb0r_t actionData, ActionEvent* actionEvent); //Not intended for direct use
  // bool GetActionsFromOffset(uint16_t offset, cb0r_t result);
  void InitializeLayer(uint8_t layer = 255); // 255 means top layer
  void DeinitializeLayer(uint8_t layer = 255);

  // Action API
  bool SetRegister(ActionInfo* actionInfo, uint32_t value);
  bool GetRegister(ActionInfo* actionInfo, uint32_t* value);
  bool ClearRegister(ActionInfo* actionInfo);
  enum LayerInfoType { ACTIVE, PASSTHROUGH };
  void SetLayerState(uint8_t layer, LayerInfoType type, bool state);
  bool GetLayerState(uint8_t layer, LayerInfoType type);

  // Helpers
  static int8_t IndexInBitmap(uint64_t bitmap, uint8_t index);  // Note this one has +1 offset (Because usually used in array index look up)
  uint8_t GetTopLayer();

  bool loaded = false;
  Dimension mapSize;
  uint8_t layerCount = 0;
  uint16_t layerEnabled = 1;
  uint16_t layerPassthrough = 0xFFFF;
  size_t uadSize;
  uint8_t* uad;

  private:
  vector<uint32_t> actionList;
  vector<uint32_t> effectList;
  uint16_t** actionLUT;
  uint16_t* effectLUT;

  std::map<uint32_t, uint32_t> registers;

  // UAD Loader
  bool CheckVersion(cb0r_t uadMap);
  bool LoadActionList(cb0r_t uadMap);
  bool LoadEffectList(cb0r_t uadMap);
  bool CreateHashList(cb0r_t cborArray, vector<uint32_t>* list); // Used to generate hash for action names
  bool CreateActionLUT(cb0r_t actionMatrix, uint16_t*** lut, Dimension lutSize);
  bool CreateEffectLUT(cb0r_t effectMatrix, uint16_t** lut, uint8_t lutSize);
  bool LoadDevice(cb0r_t uadMap);
};

#define IsBitSet(byte, bit) ((byte & (1 << bit)) != 0)