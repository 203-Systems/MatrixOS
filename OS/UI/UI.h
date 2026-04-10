#pragma once

#include "MatrixOS.h"
#include <string>
#include <list>
#include <map>
#include <algorithm>
#include <stdarg.h>
#include "UICallback.h"
#include "UIComponents.h"
#include "UIUtilities.h"

#define UI_DEFAULT_MAX_FPS 100

class UI {
public:
  string name;
  Color nameColor;

  UI() {};
  UI(string name, Color color = Color::White, bool newLEDLayer = true);
  virtual ~UI();

  void Start();

  void SetName(string name);
  void SetColor(Color color);
  void ShouldCreatenewLEDLayer(bool create);

  template <typename F> void SetSetupFunc(F&& f) { setup_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetLoopFunc(F&& f) { loop_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetGlobalLoopFunc(F&& f) { global_loop_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetEndFunc(F&& f) { end_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetPreRenderFunc(F&& f) { pre_render_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetPostRenderFunc(F&& f) { post_render_func = UICallback<void()>(static_cast<F&&>(f)); }
  template <typename F> void SetInputEventHandler(F&& f) { inputEventHandler = UICallback<bool(InputEvent*)>(static_cast<F&&>(f)); }

  void AddUIComponent(UIComponent* uiComponent, Point xy);
  void ClearUIComponents();

  void AllowExit(bool allow);

  void SetFPS(uint16_t fps);

  static void GlobalLoops();

  void Exit();

  static void ExitAllUIs();

  // Returns true if any live UI holds a raw pointer to this component.
  // Used by Python binding Close() to prevent dangling-pointer destruction.
  static bool IsComponentAttached(UIComponent* component);

private:
  int8_t status = 0;

  bool newLEDLayer = true;
  bool disableExit = false;
  bool needRender = false;

  Timer uiTimer;
  uint32_t uiUpdateMS = 1000 / UI_DEFAULT_MAX_FPS;

  UICallback<void()> setup_func;
  UICallback<void()> loop_func;
  UICallback<void()> global_loop_func;
  UICallback<void()> pre_render_func;
  UICallback<void()> post_render_func;
  UICallback<void()> end_func;
  UICallback<bool(InputEvent*)> inputEventHandler;

  std::list<pair<Point, UIComponent*>> uiComponents;
  int8_t prev_layer = -1;
  int8_t current_layer = -1;

  virtual void Setup();
  virtual void Loop();
  virtual void GlobalLoop();
  virtual void PreRender();
  virtual void PostRender();
  virtual void End();

  void LoopTask();

  void GetKey();

  virtual bool CustomInputEvent(InputEvent* inputEvent) {
    if (inputEventHandler)
      return inputEventHandler(inputEvent);
    return false;
  }; // Return true to skip UIKeyEvent

  void RenderUI();
  void UIEnd();
  void UIKeyEvent(InputEvent* inputEvent);
  void PostCallbackCleanUp();

  static std::vector<UI*> uiList;
  static void RegisterUI(UI* ui);
  static void UnregisterUI(UI* ui);
};