#pragma once

#define SYSTEM_VAR_NAMESPACE "SYSTEM_VAR"

class Application;
class Application_Info;

namespace MatrixOS::SYS
{
  inline StackType_t application_stack[APPLICATION_STACK_SIZE];
  inline StaticTask_t application_taskdef;

  inline StackType_t supervisor_stack[configMINIMAL_STACK_SIZE * 4];
  inline StaticTask_t supervisor_taskdef;

  inline bool inited = false;
  inline Application* active_app = NULL;
  inline TaskHandle_t active_app_task = NULL;
  inline uint32_t active_app_id = 0;
  inline Application_Info* active_app_info = NULL;
  inline uint32_t next_app_id = 0;

  inline SavedVariable<uint32_t> prev_system_version = SavedVariable<uint32_t>(SYSTEM_VAR_NAMESPACE, "PREV_OS_VERSION", MATRIXOS_VERSION_ID);

  void Begin(void);

  void InitSysModules(void);

  uint32_t GenerateAPPID(string author, string app_name);

  uint16_t GetApplicationCount();

  void UpdateSystemNVS();
}