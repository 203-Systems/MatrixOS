#include "UI.h"

std::vector<UI*> UI::uiList;

UI::UI(string name, Color color, bool newLedLayer) {
  this->name = name;
  this->nameColor = color;
  this->newLedLayer = newLedLayer;

  UI::RegisterUI(this);
}

UI::~UI() {
  UI::UnregisterUI(this);
}

void UI::Start() {
  status = 0;
  if (newLedLayer)
  {
    prev_layer = MatrixOS::LED::CurrentLayer();
    current_layer = MatrixOS::LED::CreateLayer();
  }
  else
  {
    prev_layer = 0;
    current_layer = MatrixOS::LED::CurrentLayer();
    MatrixOS::LED::Fade();
  }
  
  MatrixOS::KeyPad::Clear();
  Setup();
  while (status != -1)
  {
    LoopTask();
    Loop();
    RenderUI();
    taskYIELD();
  }
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
    MatrixOS::LED::Fill(0);
    PreRender();
    for (auto const& uiComponentPair : uiComponents)
    {
      if (uiComponentPair.second->IsEnabled() == false) { continue; }
      Point xy = uiComponentPair.first;
      UIComponent* uiComponent = uiComponentPair.second;
      uiComponent->Render(xy);
    }
    PostRender();
    MatrixOS::LED::Update();
  }
}

void UI::GetKey() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KeyPad::Get(&keyEvent))
  {
    if (!CustomKeyEvent(&keyEvent)) //Run Custom Key Event first. Check if UI event is blocked
    {
      UIKeyEvent(&keyEvent);
    }
  }
}

void UI::UIKeyEvent(KeyEvent* keyEvent) {
  // MLOGD("UI Key Event", "%d - %d", keyID, keyInfo->state);
  if (keyEvent->id == FUNCTION_KEY)
  {
    if (!disableExit && keyEvent->info.state == RELEASED)
    {
      MLOGD("UI", "Function Key Exit");
      Exit();
      return;
    }
  }
  Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->id);
  if (xy)
  {
    bool hasAction = false;
    for (auto it = uiComponents.rbegin(); it != uiComponents.rend(); ++it)
    {
      if (it->second->IsEnabled() == false) { continue; }
      Point relative_xy = xy - it->first;
      UIComponent* uiComponent = it->second;
      if (uiComponent->GetSize().Contains(relative_xy))  // Key Found
      { hasAction |= uiComponent->KeyEvent(relative_xy, &keyEvent->info); }
      if (hasAction) { break; }
    }
    if (this->name.empty() == false && hasAction == false && keyEvent->info.state == HOLD && Dimension(Device::x_size, Device::y_size).Contains(xy))
    { MatrixOS::UIUtility::TextScroll(this->name, this->nameColor); }
  }
}

void UI::AddUIComponent(UIComponent* uiComponent, Point xy) {
  uiComponents.push_back(pair<Point, UIComponent*>(xy, uiComponent));
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

void UI::SetPreRenderFunc(std::function<void()> pre_render_func) {
  UI::pre_render_func = &pre_render_func;
}

void UI::SetPostRenderFunc(std::function<void()> post_render_func) {
  UI::post_render_func = &post_render_func;
}

void UI::SetKeyEventHandler(std::function<bool(KeyEvent*)> key_event_handler){
  UI::key_event_handler = &key_event_handler;
}

void UI::ClearUIComponents() {
  uiComponents.clear();
}

void UI::UIEnd() {
  // Check if UI is already exited
  if(status == INT8_MIN) { return; }

  End();
  MLOGD("UI", "UI %s Exited", name.c_str());

  MatrixOS::KeyPad::Clear();

  if (newLedLayer)
  { 
    MatrixOS::LED::DestroyLayer(); }
  else
  {
    MatrixOS::LED::Fade();
  }

  status = INT8_MIN;
}

void UI::SetFPS(uint16_t fps)
{
  if (fps == 0)
  {
    uiUpdateMS = UINT32_MAX;
  }
  else
  {
    uiUpdateMS = 1000 / fps;
  }
}

void UI::RegisterUI(UI* ui) {
  MLOGD("UI", "Register UI %s", ui->name.c_str());
  UI::uiList.push_back(ui);
}

void UI::UnregisterUI(UI* ui) {
  MLOGD("UI", "Unregister UI %s", ui->name.c_str());
  auto it = std::find(UI::uiList.begin(), UI::uiList.end(), ui);
  if (it != UI::uiList.end()) {
    (*it)->ClearUIComponents();
    UI::uiList.erase(it);
  }
}

void UI::CleanUpUIs() {
  for (auto it = UI::uiList.rbegin(); it != UI::uiList.rend(); ++it)
  {
      (*it)->ClearUIComponents();
  }
  UI::uiList.clear();
}