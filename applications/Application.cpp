#include "MatrixOS.h"
#include "Application.h"
#include "./UI/UI.h"

void Application::Start(void* args) {
  this->args = args;
  Setup();
  while (true)
  {
    Loop();
  }
}

void Application::Exit() {
  End();
  UI::FadeOut();
  MatrixOS::SYS::ExitAPP();
}