#include "MatrixOS.h"
#include "Application.h"
#include "./UI/UI.h"

void Application::Start(void* args) {
  this->args = args;
  Setup();
  while (true)
  {
    Loop();
    taskYIELD();
  }
}

void Application::Exit() {
  End();
  MatrixOS::SYS::ExitAPP();
}