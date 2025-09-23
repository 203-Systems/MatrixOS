#pragma once

#include "MatrixOS.h"
#include <string>
#include <functional>
#include <list>
#include <map>
#include <stdarg.h>
#include "UIComponents.h"
#include "UIUtilities.h"

#define UI_DEFAULT_MAX_FPS 100

class UI {
 public:
  string name;
  Color nameColor;

  UI() {};
  UI(string name, Color color = Color(0xFFFFFF), bool newLEDLayer = true);
  virtual ~UI();

  void Start();

  void SetName(string name);
  void SetColor(Color color);
  void ShouldCreatenewLEDLayer(bool create);
  void SetSetupFunc(std::function<void()> setup_func);
  void SetLoopFunc(std::function<void()> loop_func);
  void SetEndFunc(std::function<void()> end_func);
  void SetPreRenderFunc(std::function<void()> pre_render_func);
  void SetPostRenderFunc(std::function<void()> post_render_func);
  void SetKeyEventHandler(std::function<bool(KeyEvent*)> key_event_handler);

  void AddUIComponent(UIComponent* uiComponent, Point xy);
  void ClearUIComponents();

  void AllowExit(bool allow);

  void SetFPS(uint16_t fps);

  void Exit();

  static void ExitAllUIs();

 private:
  int8_t status = 0;

  bool newLEDLayer = true;
  bool disableExit = false;
  bool needRender = false;

  Timer uiTimer;
  uint32_t uiUpdateMS = 1000 / UI_DEFAULT_MAX_FPS;

  std::function<void()>* setup_func = nullptr;
  std::function<void()>* loop_func = nullptr;
  std::function<void()>* pre_render_func = nullptr;
  std::function<void()>* post_render_func = nullptr;
  std::function<void()>* end_func = nullptr;
  std::function<bool(KeyEvent*)>* key_event_handler = nullptr;

  std::list<pair<Point, UIComponent*>> uiComponents;
  int8_t prev_layer = -1;
  int8_t current_layer = -1;

  virtual void Setup();
  virtual void Loop();
  virtual void PreRender();
  virtual void PostRender();
  virtual void End();

  void LoopTask();

  void GetKey();

  virtual bool CustomKeyEvent(KeyEvent* keyEvent) { 
    if (key_event_handler)
      return (*key_event_handler)(keyEvent);
    return false; 
  };  // Return true to skip UIKeyEvent


  void RenderUI();
  void UIEnd();
  void UIKeyEvent(KeyEvent* keyEvent);
  void PostCallbackCleanUp();

  static std::vector<UI*> uiList;
  static void RegisterUI(UI* ui);
  static void UnregisterUI(UI* ui);
};