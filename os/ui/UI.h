#pragma once

#include "MatrixOS.h"
#include <string>
#include <functional>
#include <list>
#include <map>
#include <stdarg.h>
#include "Component/UIComponents.h"

class UI {
 public:
  string name;
  Color nameColor;
  int8_t status = 0;

  bool newLedLayer = false;
  bool disableExit = false;

  Timer uiTimer;
  const uint16_t uiFps = 60;

  std::function<void()>* setup_func = nullptr;
  std::function<void()>* loop_func = nullptr;
  std::function<void()>* render_func = nullptr;
  std::function<void()>* end_func = nullptr;
  std::function<bool(KeyEvent*)>* key_event_handler = nullptr;
  
  UI(){};
  UI(string name, Color color, bool newLedLayer = false);

  void Start();

  virtual void Setup() {
    if (setup_func)
      (*setup_func)();
  };
  virtual void Loop() {
    if (loop_func)
      (*loop_func)();
  };
  virtual void Render() {
    if (render_func)
      (*render_func)();
  };
  virtual void End() {
    if (end_func)
      (*end_func)();
  };

  void Exit();

  void LoopTask();

  void GetKey();
  bool CustomKeyEvent(KeyEvent* keyEvent) { 
    if (key_event_handler)
      return (*key_event_handler)(keyEvent);
    return false; 
  };  // Return true to skip UIKeyEvent

  void SetSetupFunc(std::function<void()> setup_func);
  void SetLoopFunc(std::function<void()> loop_func);
  void SetEndFunc(std::function<void()> end_func);
  void SetKeyEventHandler(std::function<bool(KeyEvent*)> key_event_handler);

  std::map<Point, UIComponent*> uiComponentMap;

  void AddUIComponent(UIComponent* uiComponent, Point xy);
  void AddUIComponent(UIComponent* uiComponent, uint16_t count, ...);

  void AllowExit(bool allow);

  void ClearUIComponents();

 private:
  void RenderUI();
  void UIEnd();
  void UIKeyEvent(KeyEvent* keyEvent);
  void PostCallbackCleanUp();
};