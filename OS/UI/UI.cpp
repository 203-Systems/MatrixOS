#include "UI.h"

std::vector<UI*> UI::uiList;

UI::UI(string name, Color color, bool newLEDLayer) {
  this->name = name;
  this->nameColor = color;
  this->newLEDLayer = newLEDLayer;

  UI::RegisterUI(this);
}

UI::~UI() {
  UI::UnregisterUI(this);
}

void UI::SetName(string name) {
  this->name = name;
}

void UI::SetColor(Color color) {
  this->nameColor = color;
}

void UI::ShouldCreatenewLEDLayer(bool create) {
  this->newLEDLayer = create;
}

void UI::Start() {
  status = 0;
  if (newLEDLayer)
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

  MatrixOS::Input::ClearInputBuffer();
  Device::Input::SuppressActiveInputs();
  Setup();
  while (status >= 0)
  {
    LoopTask();
    GlobalLoops();
    Loop();
    RenderUI();
    taskYIELD();
  }
  UIEnd();
}

void UI::Exit() {
  if (status != INT8_MIN)
  {
    status = -1;
  }
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
      if (uiComponentPair.second->IsEnabled() == false)
      {
        continue;
      }
      Point xy = uiComponentPair.first;
      UIComponent* uiComponent = uiComponentPair.second;
      uiComponent->Render(xy);
    }
    PostRender();
    MatrixOS::LED::Update();
  }
}

void UI::GetKey() {
  InputEvent inputEvent;
  while (MatrixOS::Input::Get(&inputEvent))
  {
    // Only handle keypad events for now
    if (inputEvent.inputClass != InputClass::Keypad)
      continue;

    if (!CustomInputEvent(&inputEvent)) // Run Custom Key Event first. Check if UI event is blocked
    {
      UIKeyEvent(&inputEvent);
    }
  }
}

void UI::UIKeyEvent(InputEvent* inputEvent) {
  if (inputEvent->id == InputId::FunctionKey())
  {
    if (!disableExit && inputEvent->keypad.state == KeypadState::Released)
    {
      MLOGD("UI", "Function Key Exit");
      Exit();
      return;
    }
  }
  Point xy;
  if (!MatrixOS::Input::GetPosition(inputEvent->id, &xy))
  {
    return;
  }
  if (xy)
  {
    bool hasAction = false;
    for (auto it = uiComponents.rbegin(); it != uiComponents.rend(); ++it)
    {
      if (it->second->IsEnabled() == false)
      {
        continue;
      }
      Point relativeXy = xy - it->first;
      UIComponent* uiComponent = it->second;
      if (uiComponent->GetSize().Contains(relativeXy)) // Key Found
      {
        hasAction |= uiComponent->KeyEvent(relativeXy, &inputEvent->keypad);
      }
      if (hasAction)
      {
        break;
      }
    }
    if (this->name.empty() == false && hasAction == false && inputEvent->keypad.state == KeypadState::Hold &&
        MatrixOS::Input::GetPrimaryGridCluster()->dimension.Contains(xy))
    {
      MatrixOS::UIUtility::TextScroll(this->name, this->nameColor);
    }
  }
}

void UI::AddUIComponent(UIComponent* uiComponent, Point xy) {
  uiComponents.push_back(pair<Point, UIComponent*>(xy, uiComponent));
}

void UI::AllowExit(bool allow) {
  disableExit = !allow;
}

void UI::SetSetupFunc(std::function<void()> setup_func) {
  UI::setup_func = std::make_unique<std::function<void()>>(setup_func);
}

void UI::SetLoopFunc(std::function<void()> loop_func) {
  UI::loop_func = std::make_unique<std::function<void()>>(loop_func);
}

void UI::SetGlobalLoopFunc(std::function<void()> global_loop_func) {
  UI::global_loop_func = std::make_unique<std::function<void()>>(global_loop_func);
}

void UI::SetEndFunc(std::function<void()> end_func) {
  UI::end_func = std::make_unique<std::function<void()>>(end_func);
}

void UI::SetPreRenderFunc(std::function<void()> pre_render_func) {
  UI::pre_render_func = std::make_unique<std::function<void()>>(pre_render_func);
}

void UI::SetPostRenderFunc(std::function<void()> post_render_func) {
  UI::post_render_func = std::make_unique<std::function<void()>>(post_render_func);
}

void UI::SetInputEventHandler(std::function<bool(InputEvent*)> inputEventHandler) {
  UI::inputEventHandler = std::make_unique<std::function<bool(InputEvent*)>>(inputEventHandler);
}

void UI::ClearUIComponents() {
  uiComponents.clear();
}

void UI::UIEnd() {
  // Check if UI is already exited
  if (status == INT8_MIN)
  {
    return;
  }

  End();
  MLOGD("UI", "UI %s Exited", name.c_str());

  MatrixOS::Input::ClearInputBuffer();
  uiComponents.clear();

  if (newLEDLayer)
  {
    MatrixOS::LED::DestroyLayer();
  }
  else
  {
    MatrixOS::LED::Fade();
  }

  status = INT8_MIN;
}

void UI::SetFPS(uint16_t fps) {
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

bool UI::IsComponentAttached(UIComponent* component) {
  for (UI* ui : uiList) {
    for (auto& pair : ui->uiComponents) {
      if (pair.second == component)
        return true;
    }
  }
  return false;
}

void UI::UnregisterUI(UI* ui) {
  MLOGD("UI", "Unregister UI %s", ui->name.c_str());
  auto it = std::find(UI::uiList.begin(), UI::uiList.end(), ui);
  if (it != UI::uiList.end())
  {
    (*it)->ClearUIComponents();
    UI::uiList.erase(it);
  }
}

void UI::GlobalLoops() {
  for (UI* ui : uiList)
  {
    ui->GlobalLoop();
  }
}

void UI::ExitAllUIs() {
  auto uiListCopy = UI::uiList;

  for (auto it = uiListCopy.rbegin(); it != uiListCopy.rend(); ++it)
  {
    if (*it != nullptr)
    {
      (*it)->status = INT8_MIN;
      (*it)->uiComponents.clear();
    }
  }
  MatrixOS::Input::ClearInputBuffer();
  UI::uiList.clear();
}

// Virtual function implementations
void UI::Setup() {
  if (setup_func)
    (*setup_func)();
}

void UI::Loop() {
  if (loop_func)
    (*loop_func)();
}

void UI::GlobalLoop() {
  if (global_loop_func)
    (*global_loop_func)();
}

void UI::PreRender() {
  if (pre_render_func)
    (*pre_render_func)();
}

void UI::PostRender() {
  if (post_render_func)
    (*post_render_func)();
}

void UI::End() {
  if (end_func)
    (*end_func)();
}
