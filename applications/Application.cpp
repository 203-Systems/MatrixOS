#include "MatrixOS.h"
#include "Application.h"

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
  MatrixOS::SYS::ExitAPP();
}