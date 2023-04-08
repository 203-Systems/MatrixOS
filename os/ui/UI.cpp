#include "UI.h"

UI::UI(string name, Color color, bool newLedLayer) {
  this->name = name;
  this->nameColor = color;
  this->newLedLayer = newLedLayer;
}

// TODO, make new led layer
void UI::Start() {
  status = 0;
  if (newLedLayer)
    MatrixOS::LED::CreateLayer();
  MatrixOS::KEYPAD::Clear();
  Setup();
  while (status != -1)
  {
    LoopTask();
    Loop();
    RenderUI();
  }
  End();
  UIEnd();
}

void UI::Exit() {
  MLOGD("UI", "UI Exit signaled");
  status = -1;
}

void UI::LoopTask() {
  GetKey();
}

void UI::RenderUI() {
  if (uiTimer.Tick(uiUpdateMS) || needRender)
  {
    needRender = false;
    // MatrixOS::LED::Fill(0);
    for (auto const& uiComponentPair : uiComponentMap)
    {
      Point xy = uiComponentPair.first;
      UIComponent* uiComponent = uiComponentPair.second;
      uiComponent->Render(xy);
    }
    Render();
    MatrixOS::LED::Update();
  }
}

void UI::GetKey() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  {
    // MLOGD("UI", "Key Event %d %d", keyEvent.id, keyEvent.info.state);
    if (!CustomKeyEvent(&keyEvent)) //Run Custom Key Event first. Check if UI event is blocked
      UIKeyEvent(&keyEvent);
    else
      MLOGD("UI", "KeyEvent Skip: %d", keyEvent.id);
  }
}

void UI::UIKeyEvent(KeyEvent* keyEvent) {
  // MLOGD("UI Key Event", "%d - %d", keyID, keyInfo->state);
  if (keyEvent->id == FUNCTION_KEY)
  {
    if (!disableExit && keyEvent->info.state == PRESSED)
    {
      MLOGD("UI", "Function Key Exit");
      Exit();
      return;
    }
  }
  Point xy = MatrixOS::KEYPAD::ID2XY(keyEvent->id);
  if (xy)
  {
    // MLOGD("UI", "UI Key Event X:%d Y:%d", xy.x, xy.y);
    bool hasAction = false;
    for (auto const& uiComponentPair : uiComponentMap)
    {
      Point relative_xy = xy - uiComponentPair.first;
      UIComponent* uiComponent = uiComponentPair.second;
      if (uiComponent->GetSize().Contains(relative_xy))  // Key Found
      { hasAction |= uiComponent->KeyEvent(relative_xy, &keyEvent->info); }
    }
    // if(hasAction)
    // { needRender = true; }
    if (this->name.empty() == false && hasAction == false && keyEvent->info.state == HOLD && Dimension(Device::x_size, Device::y_size).Contains(xy))
    { MatrixOS::UIInterface::TextScroll(this->name, this->nameColor); }
  }
}

void UI::AddUIComponent(UIComponent* uiComponent, Point xy) {
  // ESP_LOGI("Add UI Component", "%d %d %s", xy.x, xy.y, uiComponent->GetName().c_str());
  // uiComponents.push_back(uiComponent);
  uiComponentMap[xy] = uiComponent;
}

void UI::AddUIComponent(UIComponent* uiComponent, uint16_t count, ...) {
  // uiComponents.push_back(uiComponent);
  va_list valst;
  va_start(valst, count);
  for (uint8_t i = 0; i < count; i++)
  { uiComponentMap[(Point)va_arg(valst, Point)] = uiComponent; }
}

void UI::AllowExit(bool allow) {
  disableExit = !allow;
}

void UI::SetSetupFunc(std::function<void()> setup_func) {
  UI::setup_func = &setup_func;
}

void UI::SetLoopFunc(std::function<void()> loop_func) {
  UI::loop_func = &loop_func;
}

void UI::SetEndFunc(std::function<void()> end_func) {
  UI::end_func = &end_func;
}

void UI::SetKeyEventHandler(std::function<bool(KeyEvent*)> key_event_handler){
  UI::key_event_handler = &key_event_handler;
}

void UI::ClearUIComponents() {
  uiComponentMap.clear();
}

void UI::UIEnd() {
  MLOGD("UI", "UI Exited");
  if (newLedLayer)
  { MatrixOS::LED::DestoryLayer(); }
  else
  { MatrixOS::LED::Fill(0); }

  MatrixOS::KEYPAD::Clear();
  // MatrixOS::LED::Update();
}

void UI::SetFPS(uint16_t fps)
{
  if (fps == 0)
    uiUpdateMS = UINT32_MAX;
  else
    uiUpdateMS = 1000 / fps;
}