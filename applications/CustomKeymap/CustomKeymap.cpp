#include "CustomKeymap.h"
#include "sample.h"
#include "UI/UI.h"
#include "applications/BrightnessControl/BrightnessControl.h"

void CustomKeymap::Setup() {
  if(!uad.LoadUAD((uint8_t*)sample_uad, sizeof(sample_uad)))
  {
    MLOGE("CustomKeymap", "Failed to load UAD");
  }
}

void CustomKeymap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}

void CustomKeymap::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  // Reserve Function Key 
  if (keyID == FUNCTION_KEY && keyInfo->state == (menuLock ? HOLD : PRESSED))
  {
    ActionMenu(); 
  }

  if(keyInfo->state == KeyState::AFTERTOUCH) { return; } // Ignore Aftertouch for now for easy of debug. Remove later
  uad.KeyEvent(keyID, keyInfo);
}

void CustomKeymap::Reload()
{
  MatrixOS::SYS::ExecuteAPP(info.author, info.name); // Just relaunch the APP lol
}

void CustomKeymap::ActionMenu() {
  MLOGD("CustomKeymap", "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButtonLarge brightnessBtn(
      "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::SYS::NextBrightness(); },
      [&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButtonLarge clearCanvasBtn("Reset", Color(0x00FF00), Dimension(2, 1), [&]() -> void { Reload(); });
  actionMenu.AddUIComponent(clearCanvasBtn, Point(3, 2));

  UIButtonLarge rotateRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1),
                              [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

  UIButton systemSettingBtn("System Setting", Color(0xFFFFFF), [&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 5));

  UIButtonDimmable menuLockBtn(
      "Menu Lock", Color(0xA0FF00), [&]() -> bool { return menuLock; }, [&]() -> void { menuLock = !menuLock; });
  actionMenu.AddUIComponent(menuLockBtn, Point(0, 5));  // Current the currentKeymap is directly linked to
                                                        // compatibilityMode. Do we really need > 2 keymap tho? 

  UILayerControl layerControl("Activated Layers", Color(0x00FFFF), Dimension(8, 2), &uad, UAD::LayerInfoType::ACTIVE);
  actionMenu.AddUIComponent(layerControl, Point(0, 0));

  UILayerControl passthroughControl("Layer Passthrough", Color(0xFF00FF), Dimension(8, 2), &uad, UAD::LayerInfoType::PASSTHROUGH);                       
  actionMenu.AddUIComponent(passthroughControl, Point(0, 6));                                        

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool { 
    if(keyEvent->id == FUNCTION_KEY)
    {
      if(keyEvent->info.state == HOLD)
      { Exit(); }
      else if(keyEvent->info.state == RELEASED)
      { actionMenu.Exit(); }
      return true; //Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
   });

  actionMenu.Start();

  MLOGD("CustomKeymap", "Exit Action Menu");
}
