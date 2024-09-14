#include "MatrixOS.h"
#include "Application.h"
#include "./UI/UI.h"

void Application::Start(void* args) {
  this->args = args;
  MatrixOS::LED::CreateLayer();
  Setup();
  while (true)
  {
    Loop();
    taskYIELD();
  }
}

void Application::Exit() {
  End();
  if(MatrixOS::UserVar::ui_animation)
  {
    MatrixOS::LED::Fade();
    MatrixOS::LED::Fill(0, 0);
    // Add a delay so it fade to black
    // This give the user a better sense that they just exited an APP
    MatrixOS::SYS::DelayMs(crossfade_duration);
  }
  MatrixOS::SYS::ExitAPP();
}