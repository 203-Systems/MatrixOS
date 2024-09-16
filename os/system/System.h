#pragma once

// #define XSTR(x) STR(x)
// #define STR(x) #x
// #pragma message "app index" XSTR(APPLICATION_INDEX)
// const inline uint16_t app_count = APPLICATION_INDEX;

#define SYSTEM_VAR_NAMESPACE "SYSTEM_VAR"

namespace MatrixOS::SYS
{
  StaticTimer_t device_task_tmdef;
  TimerHandle_t device_task_tm;

  StackType_t application_stack[APPLICATION_STACK_SIZE];
  StaticTask_t application_taskdef;

  StackType_t supervisor_stack[configMINIMAL_STACK_SIZE * 4];
  StaticTask_t supervisor_taskdef;

  inline Application* active_app = NULL;
  inline TaskHandle_t active_app_task = NULL;
  inline uint32_t active_app_id = 0;
  inline Application_Info* active_app_info = NULL;
  inline uint32_t next_app_id = 0;

  SavedVariable<uint32_t> prev_system_version = SavedVariable<uint32_t>(SYSTEM_VAR_NAMESPACE, "PREV_OS_VERSION", MATRIXOS_VERSION_ID);

  void ExecuteAPP(uint32_t app_id);
  uint32_t GenerateAPPID(string author, string app_name);

  uint16_t GetApplicationCount();

  void UpdateSystemNVS();
}