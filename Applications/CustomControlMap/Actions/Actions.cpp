#include "../UAD.h"

// Actions
#include "midi/MidiAction.h"
#include "keyboard/KeyboardAction.h"
#include "layer/LayerAction.h"
#include "wrap/WrapAction.h"
#include "gamepad/GamepadAction.h"

// Effects
#include "color/ColorEffect.h"
#include "actioncolor/ActionColorEffect.h"

#define TAG "UAD Actions"

bool UADRuntime::ExecuteAction(ActionInfo* actionInfo, cb0r_t actionData, ActionEvent* actionEvent) {
  if (actionInfo->depth > 5)
  {
    MLOGE(TAG, "Action depth exceeded");
    return false;
  }

  cb0r_s actionIndex;

  if (!cb0r_get_check_type(actionData, 0, &actionIndex, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get action index");
    return false;
  }

  uint32_t actionSignature = 0;

  if (actionInfo->actionType == ActionType::ACTION)
  {
    if (actionIndex.value >= actionList.size())
    {
      MLOGE(TAG, "Action index out of range: %d (size=%d)", actionIndex.value, actionList.size());
      return false;
    }
    actionSignature = actionList[actionIndex.value];
    MLOGV(TAG, "Executing action - %d", actionSignature);
  }
  else if (actionInfo->actionType == ActionType::EFFECT)
  {
    if (actionIndex.value >= effectList.size())
    {
      MLOGE(TAG, "Effect index out of range: %d (size=%d)", actionIndex.value, effectList.size());
      return false;
    }
    actionSignature = effectList[actionIndex.value];
    MLOGV(TAG, "Executing effect - %d", actionSignature);
  }

  if (actionEvent->type == ActionEventType::KEYEVENT)
  {
    switch (actionSignature)
    {
    case MidiAction::signature:
      return MidiAction::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case KeyboardAction::signature:
      return KeyboardAction::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case GamepadAction::signature:
      return GamepadAction::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case LayerAction::signature:
      return LayerAction::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case WrapAction::signature:
      return WrapAction::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case ColorEffect::signature:
      return ColorEffect::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    case ActionColorEffect::signature:
      return ActionColorEffect::KeyEvent(this, actionInfo, actionData, actionEvent->keyInfo);
    }
  }
  else if (actionEvent->type == ActionEventType::INITIALIZATION)
  {
    switch (actionSignature)
    {
    case ColorEffect::signature:
      return ColorEffect::Initialization(this, actionInfo, actionData);
    case ActionColorEffect::signature:
      return ActionColorEffect::Initialization(this, actionInfo, actionData);
    }
  }
  return false;
}
